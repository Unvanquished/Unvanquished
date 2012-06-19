/*
	common.h

	Common header file for dpmaster

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


#ifndef _COMMON_H_
#define _COMMON_H_


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifdef WIN32
# include <winsock2.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <sys/socket.h>
#endif

#if defined __GNUC__ || defined __clang__
#define NORETURN __attribute__((__noreturn__))
#define UNUSED __attribute__((__unused__))
#define PRINTF_ARGS(f, a) __attribute__((__format__(__printf__, (f), (a))))
#define PRINTF_LIKE(n) PRINTF_ARGS((n), (n) + 1)
#define VPRINTF_LIKE(n) PRINTF_ARGS((n), 0)
#define ALIGNED(a) __attribute__((__aligned__(a)))
#define ALWAYS_INLINE __attribute__((__always_inline__))
#else
#define NORETURN
#define UNUSED
#define PRINTF_ARGS(f, a)
#define PRINTF_LIKE(n)
#define VPRINTF_LIKE(n)
#define ALIGNED(a)
#define ALWAYS_INLINE
#define __attribute__(x)
#endif

// ---------- Types ---------- //

// A few basic types
typedef enum {qfalse, qtrue} qboolean;
typedef unsigned char qbyte;

// The various messages levels
typedef enum
{
	MSG_NOPRINT,	// used by "max_msg_level" (= no printings)
	MSG_ERROR,		// errors
	MSG_WARNING,	// warnings
	MSG_NORMAL,		// standard messages
	MSG_DEBUG		// for debugging purpose
} msg_level_t;


// ---------- Public variables ---------- //

// The master socket
extern int inSock;
extern int outSock;

// The current time (updated every time we receive a packet)
extern time_t crt_time;

// Maximum level for a message to be printed
extern msg_level_t max_msg_level;

// Peer address. We rebuild it every time we receive a new packet
extern char peer_address [128];


// ---------- Public functions ---------- //

// Win32 uses a different name for some standard functions
#ifdef WIN32
# define snprintf _snprintf
#endif

// Print a message to screen, depending on its verbose level
int MsgPrint (msg_level_t msg_level, const char* format, ...) PRINTF_LIKE(2);

void RecordClientStat( const char *address, const char *version, const char *renderer );
void RecordGameStat( const char *address, const char *dataText );

#endif  // #ifndef _COMMON_H_
