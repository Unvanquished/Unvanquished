/*
	messages.c

	Message management for tremmaster

	Copyright (C) 2004  Mathieu Olivier

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "common.h"
#include "messages.h"
#include "servers.h"


// ---------- Constants ---------- //

// Timeouts (in secondes)
#define TIMEOUT_HEARTBEAT		2
#define TIMEOUT_INFORESPONSE	(15 * 60)

// Period of validity for a challenge string (in secondes)
#define TIMEOUT_CHALLENGE 2

// Maximum size of a reponse packet
#define MAX_PACKET_SIZE 1400


// Types of messages (with samples):

// "heartbeat Tremulous\n"
#define S2M_HEARTBEAT "heartbeat"

// "gamestat <data>"
#define S2M_GAMESTAT "gamestat"

// "getinfo A_Challenge"
#define M2S_GETINFO "getinfo"

// "infoResponse\n\\pure\\1\\..."
#define S2M_INFORESPONSE "infoResponse\x0A"

// "getservers 67 empty full"
#define C2M_GETSERVERS "getservers "

// "getserversResponse\\...(6 bytes)...\\...(6 bytes)...\\EOT\0\0\0"
#define M2C_GETSERVERSREPONSE "getserversResponse"

#define C2M_GETMOTD "getmotd"
#define M2C_MOTD    "motd "


// ---------- Private functions ---------- //

/*
====================
SearchInfostring

Search an infostring for the value of a key
====================
*/
static char* SearchInfostring (const char* infostring, const char* key)
{
	static char value [256];
	char crt_key [256];
	size_t value_ind, key_ind;
	char c;

	if (*infostring++ != '\\')
		return NULL;

	value_ind = 0;
	for (;;)
	{
		key_ind = 0;

		// Get the key name
		for (;;)
		{
			c = *infostring++;

			if (c == '\0')
				return NULL;
			if (c == '\\' || key_ind == sizeof (crt_key) - 1)
			{
				crt_key[key_ind] = '\0';
				break;
			}

			crt_key[key_ind++] = c;
		}

		// If it's the key we are looking for, save it in "value"
		if (!strcmp (crt_key, key))
		{
			for (;;)
			{
				c = *infostring++;

				if (c == '\0' || c == '\\' || value_ind == sizeof (value) - 1)
				{
					value[value_ind] = '\0';
					return value;
				}

				value[value_ind++] = c;
			}
		}

		// Else, skip the value
		for (;;)
		{
			c = *infostring++;

			if (c == '\0')
				return NULL;
			if (c == '\\')
				break;
		}
	}
}


/*
====================
BuildChallenge

Build a challenge string for a "getinfo" message
====================
*/
static const char* BuildChallenge (void)
{
	static char challenge [CHALLENGE_MAX_LENGTH];
	size_t ind;
	size_t length = CHALLENGE_MIN_LENGTH - 1;  // We start at the minimum size

	// ... then we add a random number of characters
	length += rand () % (CHALLENGE_MAX_LENGTH - CHALLENGE_MIN_LENGTH + 1);

	for (ind = 0; ind < length; ind++)
	{
		char c;
		do
		{
			c = 33 + rand () % (126 - 33 + 1);  // -> c = 33..126
		} while (c == '\\' || c == ';' || c == '"' || c == '%' || c == '/');

		challenge[ind] = c;
	}

	challenge[length] = '\0';
	return challenge;
}


/*
====================
SendGetInfo

Send a "getinfo" message to a server
====================
*/
static void SendGetInfo (server_t* server)
{
	char msg [64] = "\xFF\xFF\xFF\xFF" M2S_GETINFO " ";

	if (!server->challenge_timeout || server->challenge_timeout < crt_time)
	{
		strncpy (server->challenge, BuildChallenge (),
				 sizeof (server->challenge) - 1);
		server->challenge_timeout = crt_time + TIMEOUT_CHALLENGE;
	}

	strncat (msg, server->challenge, sizeof (msg) - strlen (msg) - 1);
	sendto (outSock, msg, strlen (msg), 0,
			(const struct sockaddr*)&server->address,
			sizeof (server->address));

	MsgPrint (MSG_DEBUG, "%s <--- getinfo with challenge \"%s\"\n",
			  peer_address, server->challenge);
}


/*
====================
HandleGetServers

Parse getservers requests and send the appropriate response
====================
*/
static void HandleGetServers (const char* msg, const struct sockaddr_in* addr)
{
	const char* packetheader = "\xFF\xFF\xFF\xFF" M2C_GETSERVERSREPONSE "\\";
	const size_t headersize = strlen (packetheader);
	char packet [MAX_PACKET_SIZE];
	size_t packetind;
	server_t* sv;
	unsigned int protocol;
	unsigned int sv_addr;
	unsigned short sv_port;
	qboolean no_empty;
	qboolean no_full;
	unsigned int numServers = 0;

	// Check if there's a name before the protocol number
	// In this case, the message comes from a DarkPlaces-compatible client
	protocol = atoi (msg);

	MsgPrint (MSG_NORMAL, "%s ---> getservers( protocol version %d )\n",
			peer_address, protocol );

	no_empty = (strstr (msg, "empty") == NULL);
	no_full = (strstr (msg, "full") == NULL);

	// Initialize the packet contents with the header
	packetind = headersize;
	memcpy(packet, packetheader, headersize);

	// Add every relevant server
	for (sv = Sv_GetFirst (); /* see below */;  sv = Sv_GetNext ())
	{
		// If we're done, or if the packet is full, send the packet
		if (sv == NULL || packetind > sizeof (packet) - (7 + 6))
		{
			// End Of Transmission
			packet[packetind    ] = 'E';
			packet[packetind + 1] = 'O';
			packet[packetind + 2] = 'T';
			packet[packetind + 3] = '\0';
			packet[packetind + 4] = '\0';
			packet[packetind + 5] = '\0';
			packetind += 6;

			// Send the packet to the client
			sendto (inSock, packet, packetind, 0, (const struct sockaddr*)addr,
					sizeof (*addr));

			MsgPrint (MSG_DEBUG, "%s <--- getserversResponse (%u servers)\n",
						peer_address, numServers);

			// If we're done
			if (sv == NULL)
				return;
			
			// Reset the packet index (no need to change the header)
			packetind = headersize;
		}

		sv_addr = ntohl (sv->address.sin_addr.s_addr);
		sv_port = ntohs (sv->address.sin_port);

		// Extra debugging info
		if (max_msg_level >= MSG_DEBUG)
		{
			MsgPrint (MSG_DEBUG,
					  "Comparing server: IP:\"%u.%u.%u.%u:%hu\", p:%u, c:%hu\n",
					  sv_addr >> 24, (sv_addr >> 16) & 0xFF,
					  (sv_addr >>  8) & 0xFF, sv_addr & 0xFF,
					  sv_port, sv->protocol, sv->nbclients );

			if (sv->protocol != protocol)
				MsgPrint (MSG_DEBUG,
						  "Reject: protocol %u != requested %u\n",
						  sv->protocol, protocol);
			if (sv->nbclients == 0 && no_empty)
				MsgPrint (MSG_DEBUG,
						  "Reject: nbclients is %hu/%hu && no_empty\n",
						  sv->nbclients, sv->maxclients);
			if (sv->nbclients == sv->maxclients && no_full)
				MsgPrint (MSG_DEBUG,
						  "Reject: nbclients is %hu/%hu && no_full\n",
						  sv->nbclients, sv->maxclients);
		}

		// Check protocol, options
		if (sv->protocol != protocol ||
			(sv->nbclients == 0 && no_empty) ||
			(sv->nbclients == sv->maxclients && no_full))
		{

			// Skip it
			continue;
		}

		// Use the address mapping associated with the server, if any
		if (sv->addrmap != NULL)
		{
			const addrmap_t* addrmap = sv->addrmap;

			sv_addr = ntohl (addrmap->to.sin_addr.s_addr);
			if (addrmap->to.sin_port != 0)
				sv_port = ntohs (addrmap->to.sin_port);

			MsgPrint (MSG_DEBUG,
					  "Server address mapped to %u.%u.%u.%u:%hu\n",
					  sv_addr >> 24, (sv_addr >> 16) & 0xFF,
					  (sv_addr >>  8) & 0xFF, sv_addr & 0xFF,
					  sv_port);
		}

		// IP address
		packet[packetind    ] =  sv_addr >> 24;
		packet[packetind + 1] = (sv_addr >> 16) & 0xFF;
		packet[packetind + 2] = (sv_addr >>  8) & 0xFF;
		packet[packetind + 3] =  sv_addr        & 0xFF;

		// Port
		packet[packetind + 4] = sv_port >> 8;
		packet[packetind + 5] = sv_port & 0xFF;

		// Trailing '\'
		packet[packetind + 6] = '\\';

		MsgPrint (MSG_DEBUG, "  - Sending server %u.%u.%u.%u:%hu\n",
				  (qbyte)packet[packetind    ], (qbyte)packet[packetind + 1],
				  (qbyte)packet[packetind + 2], (qbyte)packet[packetind + 3],
				  sv_port);

		packetind += 7;
		numServers++;
	}
}


/*
====================
HandleInfoResponse

Parse infoResponse messages
====================
*/
static void HandleInfoResponse (server_t* server, const char* msg)
{
	char* value;
	unsigned int new_protocol = 0, new_maxclients = 0;

	MsgPrint (MSG_DEBUG, "%s ---> infoResponse\n", peer_address);

	// Check the challenge
	if (!server->challenge_timeout || server->challenge_timeout < crt_time)
	{
		MsgPrint (MSG_WARNING,
				  "WARNING: infoResponse with obsolete challenge from %s\n",
				  peer_address);
		return;
	}
	value = SearchInfostring (msg, "challenge");
	if (!value || strcmp (value, server->challenge))
	{
		MsgPrint (MSG_ERROR, "ERROR: invalid challenge from %s (%s)\n",
				  peer_address, value);
		return;
	}
	// some people enjoy making it hard to find them
	value = SearchInfostring (msg, "hostname");
	if (!value || !value[0])
	{
		MsgPrint (MSG_ERROR, "ERROR: no hostname from %s\n",
				  peer_address, value);
		return;
	}

	// Check and save the values of "protocol" and "maxclients"
	value = SearchInfostring (msg, "protocol");
	if (value)
		new_protocol = atoi (value);
	value = SearchInfostring (msg, "sv_maxclients");
	if (value)
		new_maxclients = atoi (value);
	if (!new_protocol || !new_maxclients)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: invalid infoResponse from %s (protocol: %d, maxclients: %d)\n",
				  peer_address, new_protocol, new_maxclients);
		return;
	}
	server->protocol = new_protocol;
	server->maxclients = new_maxclients;

	// Save some other useful values
	value = SearchInfostring (msg, "clients");
	if (value)
		server->nbclients = atoi (value);

	// Set a new timeout
	server->timeout = crt_time + TIMEOUT_INFORESPONSE;
}


#define CHALLENGE_KEY "challenge\\"
#define MOTD_KEY      "motd\\"

/*
====================
HandleGetMotd

Parse getservers requests and send the appropriate response
====================
*/
static void HandleGetMotd( const char* msg, const struct sockaddr_in* addr )
{
	const char		*packetheader = "\xFF\xFF\xFF\xFF" M2C_MOTD "\"";
	const size_t	headersize = strlen (packetheader);
	char					packet[ MAX_PACKET_SIZE ];
	char					challenge[ MAX_PACKET_SIZE ];
	const char		*motd = ""; //FIXME
	size_t				packetind;
	char					*value;
	char		version[ 1024 ], renderer[ 1024 ];

	MsgPrint( MSG_DEBUG, "%s ---> getmotd\n", peer_address );

	value = SearchInfostring( msg, "challenge" );
	if( !value )
	{
		MsgPrint( MSG_ERROR, "ERROR: invalid challenge from %s (%s)\n",
					peer_address, value );
		return;
	}

	strncpy( challenge, value, sizeof(challenge)-1 );
	challenge[sizeof(challenge)-1] = '\0';

	value = SearchInfostring( msg, "renderer" );
	if( value )
	{
		strncpy( renderer, value, sizeof(renderer)-1 );
		renderer[sizeof(renderer)-1] = '\0';
		MsgPrint( MSG_DEBUG, "%s is using renderer %s\n", peer_address, value );
	}

	value = SearchInfostring( msg, "version" );
	if( value )
	{
		strncpy( version, value, sizeof(version)-1 );
		version[sizeof(version)-1] = '\0';
		MsgPrint( MSG_DEBUG, "%s is using version %s\n", peer_address, value );
	}

#ifndef _WIN32
  RecordClientStat( peer_address, version, renderer );
#endif

	// Initialize the packet contents with the header
	packetind = headersize;
	memcpy( packet, packetheader, headersize );

	strncpy( &packet[ packetind ], CHALLENGE_KEY, MAX_PACKET_SIZE - packetind - 2 );
	packetind += strlen( CHALLENGE_KEY );

	strncpy( &packet[ packetind ], challenge, MAX_PACKET_SIZE - packetind - 2 );
	packetind += strlen( challenge );
	packet[ packetind++ ] = '\\';

	strncpy( &packet[ packetind ], MOTD_KEY, MAX_PACKET_SIZE - packetind - 2 );
	packetind += strlen( MOTD_KEY );

	strncpy( &packet[ packetind ], motd, MAX_PACKET_SIZE - packetind - 2 );
	packetind += strlen( motd );

	if (packetind > MAX_PACKET_SIZE - 2)
		packetind = MAX_PACKET_SIZE - 2;
	
	packet[ packetind++ ] = '\"';
	packet[ packetind++ ] = '\0';

	MsgPrint( MSG_DEBUG, "%s <--- motd\n", peer_address );

	// Send the packet to the client
	sendto( inSock, packet, packetind, 0, (const struct sockaddr*)addr,
			sizeof( *addr ) );
}

/*
====================
HandleGameStat
====================
*/
static void HandleGameStat( const char* msg, const struct sockaddr_in* addr )
{
#ifndef _WIN32
  RecordGameStat( peer_address, msg );
#endif
}

// ---------- Public functions ---------- //

/*
====================
HandleMessage

Parse a packet to figure out what to do with it
====================
*/
void HandleMessage (const char* msg, size_t length,
					const struct sockaddr_in* address)
{
	server_t* server;

	// If it's an heartbeat
	if (!strncmp (S2M_HEARTBEAT, msg, strlen (S2M_HEARTBEAT)))
	{
		char gameId [64];

		// Extract the game id
		sscanf (msg + strlen (S2M_HEARTBEAT) + 1, "%63s", gameId);
		MsgPrint (MSG_DEBUG, "%s ---> heartbeat (%s)\n",
				  peer_address, gameId);

		// Get the server in the list (add it to the list if necessary)
		server = Sv_GetByAddr (address, qtrue);
		if (server == NULL)
			return;

		server->active = qtrue;

		// If we haven't yet received any infoResponse from this server,
		// we let it some more time to contact us. After that, only
		// infoResponse messages can update the timeout value.
		if (!server->maxclients)
			server->timeout = crt_time + TIMEOUT_HEARTBEAT;

		// Ask for some infos
		SendGetInfo (server);
	}

	// If it's an infoResponse message
	else if (!strncmp (S2M_INFORESPONSE, msg, strlen (S2M_INFORESPONSE)))
	{
		server = Sv_GetByAddr (address, qfalse);
		if (server == NULL)
			return;

		HandleInfoResponse (server, msg + strlen (S2M_INFORESPONSE));
	}

	// If it's a getservers request
	else if (!strncmp (C2M_GETSERVERS, msg, strlen (C2M_GETSERVERS)))
	{
		HandleGetServers (msg + strlen (C2M_GETSERVERS), address);
	}

	// If it's a getmotd request
	else if (!strncmp (C2M_GETMOTD, msg, strlen (C2M_GETMOTD)))
	{
		HandleGetMotd (msg + strlen (C2M_GETMOTD), address);
	}

  // If it's a game statistic
  else if( !strncmp( S2M_GAMESTAT, msg, strlen ( S2M_GAMESTAT ) ) )
  {
    server = Sv_GetByAddr(address, qfalse);
    if (server == NULL)
	return;
    if( crt_time - server->lastGameStat > MIN_GAMESTAT_DELAY )
      HandleGameStat( msg + strlen( S2M_GAMESTAT ), address );
    else
      MsgPrint( MSG_NORMAL, "%s flooding GAMESTAT messages\n", peer_address );
    server->lastGameStat = crt_time;
  }
}
