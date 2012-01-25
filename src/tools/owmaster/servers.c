/*
	servers.c

	Server list and address mapping management for dpmaster

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
#include "servers.h"


// ---------- Constants ---------- //

// Address hash bitmask
#define HASH_BITMASK (hash_table_size - 1)


// ---------- Variables ---------- //

// All server structures are allocated in one block in the "servers" array.
// Each used slot is also part of a linked list in "hash_table". A simple
// hash of the address and port of a server gives its index in the table.
static server_t* servers = NULL;
static size_t max_nb_servers = DEFAULT_MAX_NB_SERVERS;
static size_t nb_servers = 0;
static server_t** hash_table = NULL;
static size_t hash_table_size = (1 << DEFAULT_HASH_SIZE);

// Last allocated entry in the "servers" array.
// Used as a start index for finding a free slot in "servers" more quickly
static unsigned int last_alloc;

// Variables for Sv_GetFirst, Sv_GetNext and Sv_RemoveCurrentAndGetNext
static server_t* crt_server = NULL;
static server_t** prev_pointer = NULL;
static int crt_hash_ind = -1;

// List of address mappings. They are sorted by "from" field (IP, then port)
static addrmap_t* addrmaps = NULL;


// ---------- Private functions ---------- //

/*
====================
Sv_AddressHash

Compute the hash of a server address
====================
*/
static unsigned int Sv_AddressHash (const struct sockaddr_in* address)
{
	qbyte* addr = (qbyte*)&address->sin_addr.s_addr;
	qbyte* port = (qbyte*)&address->sin_port;
	qbyte hash;

	hash = addr[0] ^ addr[1] ^ addr[2] ^ addr[3] ^ port[0] ^ port[1];
	return hash & HASH_BITMASK;
}


/*
====================
Sv_RemoveAndGetNextPtr

Remove a server from the list and returns its "next" pointer
====================
*/
static server_t* Sv_RemoveAndGetNextPtr (server_t* sv, server_t** prev)
{
	nb_servers--;
	MsgPrint (MSG_NORMAL,
	          "%s:%hu timed out; %u servers currently registered\n",
	          inet_ntoa (sv->address.sin_addr), ntohs (sv->address.sin_port),
			  nb_servers);

	// Mark this structure as "free"
	sv->active = qfalse;

	*prev = sv->next;
	return sv->next;
}


/*
====================
Sv_ResolveAddr

Resolve an internet address
name may include a port number, after a ':'
====================
*/
static qboolean Sv_ResolveAddr (const char* name, struct sockaddr_in* addr)
{
	char *namecpy, *port;
	struct hostent* host;

	// Create a work copy
	namecpy = strdup (name);
	if (namecpy == NULL)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: can't allocate enough memory to resolve %s\n",
				  name);
		return qfalse;
	}

	// Find the port in the address
	port = strchr (namecpy, ':');
	if (port != NULL)
		*port++ = '\0';

	// Resolve the address
	host = gethostbyname (namecpy);
	if (host == NULL)
	{
		MsgPrint (MSG_ERROR, "ERROR: can't resolve %s\n", namecpy);
		free (namecpy);
		return qfalse;
	}
	if (host->h_addrtype != AF_INET)
	{
		MsgPrint (MSG_ERROR, "ERROR: %s is not an IPv4 address\n",
				  namecpy);
		free (namecpy);
		return qfalse;
	}

	// Build the structure
	memset (addr, 0, sizeof (*addr));
	addr->sin_family = AF_INET;
	memcpy (&addr->sin_addr.s_addr, host->h_addr,
			sizeof (addr->sin_addr.s_addr));
	if (port != NULL)
		addr->sin_port = htons ((unsigned short)atoi (port));

	MsgPrint (MSG_DEBUG, "\"%s\" resolved to %s:%hu\n",
			  name, inet_ntoa (addr->sin_addr), ntohs (addr->sin_port));

	free (namecpy);
	return qtrue;
}


/*
====================
Sv_InsertAddrmapIntoList

Insert an addrmap structure to the addrmaps list
====================
*/
static void Sv_InsertAddrmapIntoList (addrmap_t* new_map)
{
	addrmap_t* addrmap = addrmaps;
	addrmap_t** prev = &addrmaps;

	// Stop at the end of the list, or if the addresses become too high
	while (addrmap != NULL &&
		   addrmap->from.sin_addr.s_addr <= new_map->from.sin_addr.s_addr)
	{
		// If we found the right place
		if (addrmap->from.sin_addr.s_addr == new_map->from.sin_addr.s_addr &&
			addrmap->from.sin_port >= new_map->from.sin_port)
		{
			// If a mapping is already recorded for this address
			if (addrmap->from.sin_port == new_map->from.sin_port)
			{
				MsgPrint (MSG_WARNING,
						  "WARNING: Address %s:%hu has several mappings\n",
						  inet_ntoa (new_map->from.sin_addr),
						  ntohs (new_map->from.sin_port));

				*prev = addrmap->next;
				free (addrmap);
			}
			break;
		}

		prev = &addrmap->next;
		addrmap = addrmap->next;
	}

	// Insert it
	new_map->next = *prev;
	*prev = new_map;

	MsgPrint (MSG_NORMAL, "Address \"%s\" mapped to \"%s\" (%s:%hu)\n",
			  new_map->from_string, new_map->to_string,
			  inet_ntoa (new_map->to.sin_addr), ntohs (new_map->to.sin_port));
}


/*
====================
Sv_GetAddrmap

Look for an address mapping corresponding to addr
====================
*/
static const addrmap_t* Sv_GetAddrmap (const struct sockaddr_in* addr)
{
	const addrmap_t* addrmap = addrmaps;
	const addrmap_t* found = NULL;

	// Stop at the end of the list, or if the addresses become too high
	while (addrmap != NULL &&
		   addrmap->from.sin_addr.s_addr <= addr->sin_addr.s_addr)
	{
		// If it's the right address
		if (addrmap->from.sin_addr.s_addr == addr->sin_addr.s_addr)
		{
			// If the exact mapping isn't there
			if (addrmap->from.sin_port > addr->sin_port)
				return found;

			// If we found the exact address
			if (addrmap->from.sin_port == addr->sin_port)
				return addrmap;

			// General mapping
			// Store it in case we don't find the exact address mapping
			if (addrmap->from.sin_port == 0)
				found = addrmap;
		}

		addrmap = addrmap->next;
	}

	return found;
}


/*
====================
Sv_ResolveAddrmap

Resolve an addrmap structure and check the parameters validity
====================
*/
static qboolean Sv_ResolveAddrmap (addrmap_t* addrmap)
{
	// Resolve the addresses
	if (!Sv_ResolveAddr (addrmap->from_string, &addrmap->from) ||
		!Sv_ResolveAddr (addrmap->to_string, &addrmap->to))
		return qfalse;

	// 0.0.0.0 addresses are forbidden
	if (addrmap->from.sin_addr.s_addr == 0 ||
		addrmap->to.sin_addr.s_addr == 0)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: Mapping from or to 0.0.0.0 is forbidden\n");
		return qfalse;
	}

	// Do NOT allow mapping to loopback addresses
	if ((ntohl (addrmap->to.sin_addr.s_addr) >> 24) == 127)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: Mapping to a loopback address is forbidden\n");
		return qfalse;
	}

	return qtrue;
}


/*
====================
Sv_IsActive

Return qtrue if a server is active.
Test if the server has timed out and remove it if it's the case.
====================
*/
static qboolean Sv_IsActive (server_t* server)
{

	// If the entry isn't even used
	if (! server->active)
		return qfalse;

	// If the server has timed out
	if (server->timeout < crt_time)
	{
		unsigned int hash;
		server_t **prev, *sv;

		hash = Sv_AddressHash (&server->address);
		prev = &hash_table[hash];
		sv = hash_table[hash];

		while (sv != server)
		{
			prev = &sv->next;
			sv = sv->next;
		}

		Sv_RemoveAndGetNextPtr (sv, prev);
		return qfalse;
	}

	return qtrue;
}


// ---------- Public functions (servers) ---------- //

/*
====================
Sv_SetHashSize

Set a new hash size value
====================
*/
qboolean Sv_SetHashSize (unsigned int size)
{
	// Too late?
	if (hash_table != NULL)
		return qfalse;

	// Too big?
	if (size > MAX_HASH_SIZE)
		return qfalse;

	hash_table_size = 1 << size;
	return qtrue;
}


/*
====================
Sv_SetMaxNbServers

Set a new hash size value
====================
*/
qboolean Sv_SetMaxNbServers (unsigned int nb)
{
	// Too late?
	if (servers != NULL)
		return qfalse;

	max_nb_servers = nb;
	return qtrue;
}


/*
====================
Sv_Init

Initialize the server list and hash table
====================
*/
qboolean Sv_Init (void)
{
	size_t array_size;

	// Allocate "servers" and clean it
	array_size = max_nb_servers * sizeof (servers[0]);
	servers = malloc (array_size);
	if (!servers)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: can't allocate the servers array (%s)\n",
				  strerror (errno));
		return qfalse;
	}
	last_alloc = max_nb_servers - 1;
	memset (servers, 0, array_size);
	MsgPrint (MSG_NORMAL, "%u server records allocated\n", max_nb_servers);

	// Allocate "hash_table" and clean it
	array_size = hash_table_size * sizeof (hash_table[0]);
	hash_table = malloc (array_size);
	if (!hash_table)
	{
		MsgPrint (MSG_ERROR, "ERROR: can't allocate the hash table (%s)\n",
				  strerror (errno));
		free (servers);
		return qfalse;
	}
	memset (hash_table, 0, array_size);
	MsgPrint (MSG_NORMAL,
			  "Hash table allocated (%u entries)\n", hash_table_size);

	return qtrue;
}


/*
====================
Sv_GetByAddr

Search for a particular server in the list; add it if necessary
====================
*/
server_t* Sv_GetByAddr (const struct sockaddr_in* address, qboolean add_it)
{
	server_t **prev, *sv;
	unsigned int hash;
	const addrmap_t* addrmap = Sv_GetAddrmap (address);
	unsigned int startpt;

	// Allow servers on a loopback address ONLY if a mapping is defined for them
	if ((ntohl (address->sin_addr.s_addr) >> 24) == 127 && addrmap == NULL)
	{
		MsgPrint (MSG_WARNING,
				  "WARNING: server %s isn't allowed (loopback address)\n",
				  peer_address);
		return NULL;
	}

	hash = Sv_AddressHash (address);
	prev = &hash_table[hash];
	sv = hash_table[hash];

	while (sv != NULL)
	{
		// We check the timeout values while browsing this list
		if (sv->timeout < crt_time)
		{
			sv = Sv_RemoveAndGetNextPtr (sv, prev);
			continue;
		}

		// Found!
		if (sv->address.sin_addr.s_addr == address->sin_addr.s_addr &&
			sv->address.sin_port == address->sin_port)
		{
			// Put it on top of the list (it's useful because heartbeats
			// are almost always followed by infoResponses)
			*prev = sv->next;
			sv->next = hash_table[hash];
			hash_table[hash] = sv;

			return sv;
		}

		prev = &sv->next;
		sv = sv->next;
	}

	if (! add_it)
		return NULL;

	// Look for the first free entry in "servers"
	startpt = last_alloc;
	for (;;)
	{
		last_alloc = (last_alloc + 1) % max_nb_servers;

		// Free entry found?
		if (!Sv_IsActive (&servers[last_alloc]))
			break;

		// No more room
		if (last_alloc == startpt)
			return NULL;
	}
	sv = &servers[last_alloc];

	// Initialize the structure
	memset (sv, 0, sizeof (*sv));
	memcpy (&sv->address, address, sizeof (sv->address));
	sv->addrmap = addrmap;

	// Add it to the list it belongs to
	sv->next = hash_table[hash];
	hash_table[hash] = sv;
	nb_servers++;

	MsgPrint (MSG_NORMAL,
			  "New server added: %s; %u servers are currently registered\n",
			  peer_address, nb_servers);
	MsgPrint (MSG_DEBUG,
			  "  - index: %u\n"
			  "  - hash: 0x%02X\n",
			  last_alloc, hash);

	return sv;
}


/*
====================
Sv_GetFirst

Get the first server in the list
====================
*/
server_t* Sv_GetFirst (void)
{
	crt_server = NULL;
	prev_pointer = NULL;
	crt_hash_ind = -1;

	return Sv_GetNext ();
}


/*
====================
Sv_GetNext

Get the next server in the list
====================
*/
server_t* Sv_GetNext (void)
{
	for (;;)
	{
		// If there is a current server, follow the link
		if (crt_server != NULL)
		{
			prev_pointer = &crt_server->next;
			crt_server = crt_server->next;
		}

		// If we don't have the next server yet
		if (crt_server == NULL)
		{
			// Search the hash table for the next server
			while (crt_hash_ind < (int)(hash_table_size - 1))
			{
				crt_hash_ind++;

				if (hash_table[crt_hash_ind] != NULL)
				{
					crt_server = hash_table[crt_hash_ind];
					prev_pointer = &hash_table[crt_hash_ind];
					break;
				}
			}
		}

		// Did we hit the end of the list?
		if (crt_server == NULL)
			return NULL;

		// If the new current server has timed out, remove it
		if (crt_server->timeout < crt_time)
			crt_server = Sv_RemoveAndGetNextPtr (crt_server, prev_pointer);
		else
			return crt_server;
	}
}


// ---------- Public functions (address mappings) ---------- //

/*
====================
Sv_AddAddressMapping

Add an unresolved address mapping to the list
mapping must be of the form "addr1:port1=addr2:port2", ":portX" are optional
====================
*/
qboolean Sv_AddAddressMapping (const char* mapping)
{
	char *map_string, *to_ip; 
	addrmap_t* addrmap;

	// Get a working copy of the mapping string
	map_string = strdup (mapping);
	if (map_string == NULL)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: can't allocate address mapping string\n");
		return qfalse;
	}

	// Find the '='
	to_ip = strchr (map_string, '=');
	if (to_ip == NULL)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: invalid syntax in address mapping string\n");
		free (map_string);
		return qfalse;
	}
	*to_ip++ = '\0';

	// Allocate the structure
	addrmap = malloc (sizeof (*addrmap));
	if (addrmap == NULL)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: can't allocate address mapping structure\n");
		free (map_string);
		return qfalse;
	}
	memset (addrmap, 0, sizeof (*addrmap));
	addrmap->from_string = strdup (map_string);
	addrmap->to_string = strdup (to_ip);
	if (addrmap->from_string == NULL || addrmap->to_string == NULL)
	{
		MsgPrint (MSG_ERROR,
				  "ERROR: can't allocate address mapping strings\n");
		free (addrmap->to_string);
		free (addrmap->from_string);
		free (map_string);
		return qfalse;
	}

	// Add it on top of "addrmaps"
	addrmap->next = addrmaps;
	addrmaps = addrmap;

	return qtrue;
}


/*
====================
Sv_ResolveAddressMappings

Resolve the address mapping list
====================
*/
qboolean Sv_ResolveAddressMappings (void)
{
	addrmap_t* unresolved = addrmaps;
	addrmap_t* addrmap;
	qboolean succeeded = qtrue;

	addrmaps = NULL;

	while (unresolved != NULL)
	{
		// Remove it from the unresolved list
		addrmap = unresolved;
		unresolved = unresolved->next;

		// Continue the resolution, even if there's an error
		if (!Sv_ResolveAddrmap (addrmap))
		{
			free (addrmap->from_string);
			free (addrmap->to_string);
			free (addrmap);
			succeeded = qfalse;
		}
		else
			Sv_InsertAddrmapIntoList (addrmap);
	}

	return succeeded;
}
