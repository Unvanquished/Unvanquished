/*
	servers.h

	Server list and address mapping management for dpmaster

	Copyright (C) 2004-2005  Mathieu Olivier

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


#ifndef _SERVERS_H_
#define _SERVERS_H_


// ---------- Constants ---------- //

// Maximum number of servers in all lists by default
#define DEFAULT_MAX_NB_SERVERS 1024

// Address hash size in bits (between 0 and MAX_HASH_SIZE)
#define DEFAULT_HASH_SIZE	6
#define MAX_HASH_SIZE		8

// Number of characters in a challenge, including the '\0'
#define CHALLENGE_MIN_LENGTH 9
#define CHALLENGE_MAX_LENGTH 12

// Minimum number of seconds between gamestat messages per server
#define MIN_GAMESTAT_DELAY 120

// ---------- Types ---------- //

// Address mapping
typedef struct addrmap_s
{
	struct addrmap_s* next;
	struct sockaddr_in from;
	struct sockaddr_in to;
	char* from_string;
	char* to_string;
} addrmap_t;

// Server properties
typedef struct server_s
{
	struct server_s* next;
	struct sockaddr_in address;
	unsigned int protocol;
	char challenge [CHALLENGE_MAX_LENGTH];
	unsigned short nbclients;
	unsigned short maxclients;
	time_t timeout;
	time_t challenge_timeout;
	const struct addrmap_s* addrmap;
	qboolean active;
	time_t lastGameStat;
} server_t;


// ---------- Public functions (servers) ---------- //

// Will simply return "false" if called after Sv_Init
qboolean Sv_SetHashSize (unsigned int size);
qboolean Sv_SetMaxNbServers (unsigned int nb);

// Initialize the server list and hash table
qboolean Sv_Init (void);

// Search for a particular server in the list; add it if necessary
// NOTE: doesn't change the current position for "Sv_GetNext"
server_t* Sv_GetByAddr (const struct sockaddr_in* address, qboolean add_it);

// Get the first server in the list
server_t* Sv_GetFirst (void);

// Get the next server in the list
server_t* Sv_GetNext (void);


// ---------- Public functions (address mappings) ---------- //

// NOTE: this is a 2-step process because resolving address mappings directly
// during the parsing of the command line would cause several problems

// Add an unresolved address mapping to the list
// mapping must be of the form "addr1:port1=addr2:port2", ":portX" are optional
qboolean Sv_AddAddressMapping (const char* mapping);

// Resolve the address mapping list
qboolean Sv_ResolveAddressMappings (void);


#endif  // #ifndef _SERVERS_H_
