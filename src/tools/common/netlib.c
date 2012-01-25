/*
===========================================================================
Copyright (C) 1999-2006 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// netlib.c

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "netlib.h"

//++timo FIXME: the WINS_ things are not necessary, they can be made portable arther easily
char           *WINS_ErrorMessage(int error);

int             WINS_Init(void);
void            WINS_Shutdown(void);
char           *WINS_MyAddress(void);
int             WINS_Listen(int socket);
int             WINS_Accept(int socket, struct sockaddr_s *addr);
int             WINS_OpenSocket(int port);
int             WINS_OpenReliableSocket(int port);
int             WINS_CloseSocket(int socket);
int             WINS_Connect(int socket, struct sockaddr_s *addr);
int             WINS_CheckNewConnections(void);
int             WINS_Read(int socket, byte * buf, int len, struct sockaddr_s *addr);
int             WINS_Write(int socket, byte * buf, int len, struct sockaddr_s *addr);
int             WINS_Broadcast(int socket, byte * buf, int len);
char           *WINS_AddrToString(struct sockaddr_s *addr);
int             WINS_StringToAddr(char *string, struct sockaddr_s *addr);
int             WINS_GetSocketAddr(int socket, struct sockaddr_s *addr);
int             WINS_GetNameFromAddr(struct sockaddr_s *addr, char *name);
int             WINS_GetAddrFromName(char *name, struct sockaddr_s *addr);
int             WINS_AddrCompare(struct sockaddr_s *addr1, struct sockaddr_s *addr2);
int             WINS_GetSocketPort(struct sockaddr_s *addr);
int             WINS_SetSocketPort(struct sockaddr_s *addr, int port);


#define GetMemory malloc
#define FreeMemory free

#define qtrue	1
#define qfalse	0

#ifdef _DEBUG
void WinPrint(char *str, ...)
{
	va_list         argptr;
	char            text[4096];

	va_start(argptr, str);
	vsprintf(text, str, argptr);
	va_end(argptr);

	printf(text);
}
#else
void WinPrint(char *str, ...)
{
}
#endif

void Net_SetAddressPort(address_t * address, int port)
{
	sockaddr_t      addr;

	WINS_StringToAddr(address->ip, &addr);
	WINS_SetSocketPort(&addr, port);
	strcpy(address->ip, WINS_AddrToString(&addr));
}								//end of the function Net_SetAddressPort

int Net_AddressCompare(address_t * addr1, address_t * addr2)
{
#ifdef WIN32
	return _stricmp(addr1->ip, addr2->ip);
#else
	return strcasecmp(addr1->ip, addr2->ip);
#endif
}								//end of the function Net_AddressCompare

void Net_SocketToAddress(socket_t * sock, address_t * address)
{
	strcpy(address->ip, WINS_AddrToString(&sock->addr));
}								//end of the function Net_SocketToAddress

int Net_Send(socket_t * sock, netmessage_t * msg)
{
	int             size;

	size = msg->size;
	msg->size = 0;
	NMSG_WriteLong(msg, size - 4);
	msg->size = size;
	//WinPrint("Net_Send: message of size %d\n", sendmsg.size);
	return WINS_Write(sock->socket, msg->data, msg->size, NULL);
}								//end of the function Net_SendSocketReliable

int Net_Receive(socket_t * sock, netmessage_t * msg)
{
	int             curread;

	if(sock->remaining > 0)
	{
		curread = WINS_Read(sock->socket, &sock->msg.data[sock->msg.size], sock->remaining, NULL);
		if(curread == -1)
		{
			WinPrint("Net_Receive: read error\n");
			return -1;
		}						//end if
		sock->remaining -= curread;
		sock->msg.size += curread;
		if(sock->remaining <= 0)
		{
			sock->remaining = 0;
			memcpy(msg, &sock->msg, sizeof(netmessage_t));
			sock->msg.size = 0;
			return msg->size - 4;
		}						//end if
		return 0;
	}							//end if
	sock->msg.size = WINS_Read(sock->socket, sock->msg.data, 4, NULL);
	if(sock->msg.size == 0)
		return 0;
	if(sock->msg.size == -1)
	{
		WinPrint("Net_Receive: size header read error\n");
		return -1;
	}							//end if
	//WinPrint("Net_Receive: message size header %d\n", msg->size);
	sock->msg.read = 0;
	sock->remaining = NMSG_ReadLong(&sock->msg);
	if(sock->remaining == 0)
		return 0;
	if(sock->remaining < 0 || sock->remaining > MAX_NETMESSAGE)
	{
		WinPrint("Net_Receive: invalid message size %d\n", sock->remaining);
		return -1;
	}							//end if
	//try to read the message
	curread = WINS_Read(sock->socket, &sock->msg.data[sock->msg.size], sock->remaining, NULL);
	if(curread == -1)
	{
		WinPrint("Net_Receive: read error\n");
		return -1;
	}							//end if
	sock->remaining -= curread;
	sock->msg.size += curread;
	if(sock->remaining <= 0)
	{
		sock->remaining = 0;
		memcpy(msg, &sock->msg, sizeof(netmessage_t));
		sock->msg.size = 0;
		return msg->size - 4;
	}							//end if
	//the message has not been completely read yet
#ifdef _DEBUG
	printf("++timo TODO: debug the Net_Receive on big size messages\n");
#endif
	return 0;
}								//end of the function Net_Receive

socket_t       *Net_AllocSocket(void)
{
	socket_t       *sock;

	sock = (socket_t *) GetMemory(sizeof(socket_t));
	memset(sock, 0, sizeof(socket_t));
	return sock;
}								//end of the function Net_AllocSocket

void Net_FreeSocket(socket_t * sock)
{
	FreeMemory(sock);
}								//end of the function Net_FreeSocket

socket_t       *Net_Connect(address_t * address, int port)
{
	int             newsock;
	socket_t       *sock;
	sockaddr_t      sendaddr;

	// see if we can resolve the host name
	WINS_StringToAddr(address->ip, &sendaddr);

	newsock = WINS_OpenReliableSocket(port);
	if(newsock == -1)
		return NULL;

	sock = Net_AllocSocket();
	if(sock == NULL)
	{
		WINS_CloseSocket(newsock);
		return NULL;
	}							//end if
	sock->socket = newsock;

	//connect to the host
	if(WINS_Connect(newsock, &sendaddr) == -1)
	{
		Net_FreeSocket(sock);
		WINS_CloseSocket(newsock);
		WinPrint("Net_Connect: error connecting\n");
		return NULL;
	}							//end if

	memcpy(&sock->addr, &sendaddr, sizeof(sockaddr_t));
	//now we can send messages
	//
	return sock;
}								//end of the function Net_Connect

socket_t       *Net_ListenSocket(int port)
{
	int             newsock;
	socket_t       *sock;

	newsock = WINS_OpenReliableSocket(port);
	if(newsock == -1)
		return NULL;

	if(WINS_Listen(newsock) == -1)
	{
		WINS_CloseSocket(newsock);
		return NULL;
	}							//end if
	sock = Net_AllocSocket();
	if(sock == NULL)
	{
		WINS_CloseSocket(newsock);
		return NULL;
	}							//end if
	sock->socket = newsock;
	WINS_GetSocketAddr(newsock, &sock->addr);
	WinPrint("listen socket opened at %s\n", WINS_AddrToString(&sock->addr));
	//
	return sock;
}								//end of the function Net_ListenSocket

socket_t       *Net_Accept(socket_t * sock)
{
	int             newsocket;
	sockaddr_t      sendaddr;
	socket_t       *newsock;

	newsocket = WINS_Accept(sock->socket, &sendaddr);
	if(newsocket == -1)
		return NULL;

	newsock = Net_AllocSocket();
	if(newsock == NULL)
	{
		WINS_CloseSocket(newsocket);
		return NULL;
	}							//end if
	newsock->socket = newsocket;
	memcpy(&newsock->addr, &sendaddr, sizeof(sockaddr_t));
	//
	return newsock;
}								//end of the function Net_Accept

void Net_Disconnect(socket_t * sock)
{
	WINS_CloseSocket(sock->socket);
	Net_FreeSocket(sock);
}								//end of the function Net_Disconnect

void Net_StringToAddress(char *string, address_t * address)
{
	strcpy(address->ip, string);
}								//end of the function Net_StringToAddress

void Net_MyAddress(address_t * address)
{
	strcpy(address->ip, WINS_MyAddress());
}								//end of the function Net_MyAddress

int Net_Setup(void)
{
	WINS_Init();
	//
	WinPrint("my address is %s\n", WINS_MyAddress());
	//
	return qtrue;
}								//end of the function Net_Setup

void Net_Shutdown(void)
{
	WINS_Shutdown();
}								//end of the function Net_Shutdown

void NMSG_Clear(netmessage_t * msg)
{
	msg->size = 4;
}								//end of the function NMSG_Clear

void NMSG_WriteChar(netmessage_t * msg, int c)
{
	if(c < -128 || c > 127)
		WinPrint("NMSG_WriteChar: range error\n");

	if(msg->size >= MAX_NETMESSAGE)
	{
		WinPrint("NMSG_WriteChar: overflow\n");
		return;
	}							//end if
	msg->data[msg->size] = c;
	msg->size++;
}								//end of the function NMSG_WriteChar

void NMSG_WriteByte(netmessage_t * msg, int c)
{
	if(c < -128 || c > 127)
		WinPrint("NMSG_WriteByte: range error\n");

	if(msg->size + 1 >= MAX_NETMESSAGE)
	{
		WinPrint("NMSG_WriteByte: overflow\n");
		return;
	}							//end if
	msg->data[msg->size] = c;
	msg->size++;
}								//end of the function NMSG_WriteByte

void NMSG_WriteShort(netmessage_t * msg, int c)
{
	if(c < ((short)0x8000) || c > (short)0x7fff)
		WinPrint("NMSG_WriteShort: range error");

	if(msg->size + 2 >= MAX_NETMESSAGE)
	{
		WinPrint("NMSG_WriteShort: overflow\n");
		return;
	}							//end if
	msg->data[msg->size] = c & 0xff;
	msg->data[msg->size + 1] = c >> 8;
	msg->size += 2;
}								//end of the function NMSG_WriteShort

void NMSG_WriteLong(netmessage_t * msg, int c)
{
	if(msg->size + 4 >= MAX_NETMESSAGE)
	{
		WinPrint("NMSG_WriteLong: overflow\n");
		return;
	}							//end if
	msg->data[msg->size] = c & 0xff;
	msg->data[msg->size + 1] = (c >> 8) & 0xff;
	msg->data[msg->size + 2] = (c >> 16) & 0xff;
	msg->data[msg->size + 3] = c >> 24;
	msg->size += 4;
}								//end of the function NMSG_WriteLong

void NMSG_WriteFloat(netmessage_t * msg, float c)
{
	if(msg->size + 4 >= MAX_NETMESSAGE)
	{
		WinPrint("NMSG_WriteLong: overflow\n");
		return;
	}							//end if
	msg->data[msg->size] = *((int *)&c) & 0xff;
	msg->data[msg->size + 1] = (*((int *)&c) >> 8) & 0xff;
	msg->data[msg->size + 2] = (*((int *)&c) >> 16) & 0xff;
	msg->data[msg->size + 3] = *((int *)&c) >> 24;
	msg->size += 4;
}								//end of the function NMSG_WriteFloat

void NMSG_WriteString(netmessage_t * msg, char *string)
{
	if(msg->size + strlen(string) + 1 >= MAX_NETMESSAGE)
	{
		WinPrint("NMSG_WriteString: overflow\n");
		return;
	}							//end if
	strcpy(&msg->data[msg->size], string);
	msg->size += strlen(string) + 1;
}								//end of the function NMSG_WriteString

void NMSG_ReadStart(netmessage_t * msg)
{
	msg->readoverflow = qfalse;
	msg->read = 4;
}								//end of the function NMSG_ReadStart

int NMSG_ReadChar(netmessage_t * msg)
{
	if(msg->size + 1 > msg->size)
	{
		msg->readoverflow = qtrue;
		WinPrint("NMSG_ReadChar: read overflow\n");
		return 0;
	}							//end if
	msg->read++;
	return msg->data[msg->read - 1];
}								//end of the function NMSG_ReadChar

int NMSG_ReadByte(netmessage_t * msg)
{
	if(msg->read + 1 > msg->size)
	{
		msg->readoverflow = qtrue;
		WinPrint("NMSG_ReadByte: read overflow\n");
		return 0;
	}							//end if
	msg->read++;
	return msg->data[msg->read - 1];
}								//end of the function NMSG_ReadByte

int NMSG_ReadShort(netmessage_t * msg)
{
	int             c;

	if(msg->read + 2 > msg->size)
	{
		msg->readoverflow = qtrue;
		WinPrint("NMSG_ReadShort: read overflow\n");
		return 0;
	}							//end if
	c = (short)(msg->data[msg->read] + (msg->data[msg->read + 1] << 8));
	msg->read += 2;
	return c;
}								//end of the function NMSG_ReadShort

int NMSG_ReadLong(netmessage_t * msg)
{
	int             c;

	if(msg->read + 4 > msg->size)
	{
		msg->readoverflow = qtrue;
		WinPrint("NMSG_ReadLong: read overflow\n");
		return 0;
	}							//end if
	c = msg->data[msg->read]
		+ (msg->data[msg->read + 1] << 8) + (msg->data[msg->read + 2] << 16) + (msg->data[msg->read + 3] << 24);
	msg->read += 4;
	return c;
}								//end of the function NMSG_ReadLong

float NMSG_ReadFloat(netmessage_t * msg)
{
	int             c;

	if(msg->read + 4 > msg->size)
	{
		msg->readoverflow = qtrue;
		WinPrint("NMSG_ReadLong: read overflow\n");
		return 0;
	}							//end if
	c = msg->data[msg->read]
		+ (msg->data[msg->read + 1] << 8) + (msg->data[msg->read + 2] << 16) + (msg->data[msg->read + 3] << 24);
	msg->read += 4;
	return *(float *)&c;
}								//end of the function NMSG_ReadFloat

char           *NMSG_ReadString(netmessage_t * msg)
{
	static char     string[2048];
	int             l, c;

	l = 0;
	do
	{
		if(msg->read + 1 > msg->size)
		{
			msg->readoverflow = qtrue;
			WinPrint("NMSG_ReadString: read overflow\n");
			string[l] = 0;
			return string;
		}						//end if
		c = msg->data[msg->read];
		msg->read++;
		if(c == 0)
			break;
		string[l] = c;
		l++;
	} while(l < sizeof(string) - 1);
	string[l] = 0;
	return string;
}								//end of the function NMSG_ReadString


/*
===================================================================

WIN32

===================================================================
*/
#ifdef _WIN32
#include <windows.h>

#define WinError WinPrint

#define qtrue	1
#define qfalse	0

typedef struct tag_error_struct
{
	int             errnum;
	LPSTR           errstr;
} ERROR_STRUCT;

#define	NET_NAMELEN			64

char            my_tcpip_address[NET_NAMELEN];

#define	DEFAULTnet_hostport	26000

#define MAXHOSTNAMELEN		256

static int      net_acceptsocket = -1;	// socket for fielding new connections
static int      net_controlsocket;
static int      net_hostport;	// udp port number for acceptsocket
static int      net_broadcastsocket = 0;

//static qboolean ifbcastinit = qfalse;
static struct sockaddr_s broadcastaddr;

static unsigned long myAddr;

WSADATA         winsockdata;

ERROR_STRUCT    errlist[] = {
	{WSAEINTR, "WSAEINTR - Interrupted"},
	{WSAEBADF, "WSAEBADF - Bad file number"},
	{WSAEFAULT, "WSAEFAULT - Bad address"},
	{WSAEINVAL, "WSAEINVAL - Invalid argument"},
	{WSAEMFILE, "WSAEMFILE - Too many open files"},

/*
*    Windows Sockets definitions of regular Berkeley error constants
*/

	{WSAEWOULDBLOCK, "WSAEWOULDBLOCK - Socket marked as non-blocking"},
	{WSAEINPROGRESS, "WSAEINPROGRESS - Blocking call in progress"},
	{WSAEALREADY, "WSAEALREADY - Command already completed"},
	{WSAENOTSOCK, "WSAENOTSOCK - Descriptor is not a socket"},
	{WSAEDESTADDRREQ, "WSAEDESTADDRREQ - Destination address required"},
	{WSAEMSGSIZE, "WSAEMSGSIZE - Data size too large"},
	{WSAEPROTOTYPE, "WSAEPROTOTYPE - Protocol is of wrong type for this socket"},
	{WSAENOPROTOOPT, "WSAENOPROTOOPT - Protocol option not supported for this socket type"},
	{WSAEPROTONOSUPPORT, "WSAEPROTONOSUPPORT - Protocol is not supported"},
	{WSAESOCKTNOSUPPORT, "WSAESOCKTNOSUPPORT - Socket type not supported by this address family"},
	{WSAEOPNOTSUPP, "WSAEOPNOTSUPP - Option not supported"},
	{WSAEPFNOSUPPORT, "WSAEPFNOSUPPORT - "},
	{WSAEAFNOSUPPORT, "WSAEAFNOSUPPORT - Address family not supported by this protocol"},
	{WSAEADDRINUSE, "WSAEADDRINUSE - Address is in use"},
	{WSAEADDRNOTAVAIL, "WSAEADDRNOTAVAIL - Address not available from local machine"},
	{WSAENETDOWN, "WSAENETDOWN - Network subsystem is down"},
	{WSAENETUNREACH, "WSAENETUNREACH - Network cannot be reached"},
	{WSAENETRESET, "WSAENETRESET - Connection has been dropped"},
	{WSAECONNABORTED, "WSAECONNABORTED - Connection aborted"},
	{WSAECONNRESET, "WSAECONNRESET - Connection reset"},
	{WSAENOBUFS, "WSAENOBUFS - No buffer space available"},
	{WSAEISCONN, "WSAEISCONN - Socket is already connected"},
	{WSAENOTCONN, "WSAENOTCONN - Socket is not connected"},
	{WSAESHUTDOWN, "WSAESHUTDOWN - Socket has been shut down"},
	{WSAETOOMANYREFS, "WSAETOOMANYREFS - Too many references"},
	{WSAETIMEDOUT, "WSAETIMEDOUT - Command timed out"},
	{WSAECONNREFUSED, "WSAECONNREFUSED - Connection refused"},
	{WSAELOOP, "WSAELOOP - "},
	{WSAENAMETOOLONG, "WSAENAMETOOLONG - "},
	{WSAEHOSTDOWN, "WSAEHOSTDOWN - Host is down"},
	{WSAEHOSTUNREACH, "WSAEHOSTUNREACH - "},
	{WSAENOTEMPTY, "WSAENOTEMPTY - "},
	{WSAEPROCLIM, "WSAEPROCLIM - "},
	{WSAEUSERS, "WSAEUSERS - "},
	{WSAEDQUOT, "WSAEDQUOT - "},
	{WSAESTALE, "WSAESTALE - "},
	{WSAEREMOTE, "WSAEREMOTE - "},

/*
*    Extended Windows Sockets error constant definitions
*/

	{WSASYSNOTREADY, "WSASYSNOTREADY - Network subsystem not ready"},
	{WSAVERNOTSUPPORTED, "WSAVERNOTSUPPORTED - Version not supported"},
	{WSANOTINITIALISED, "WSANOTINITIALISED - WSAStartup() has not been successfully called"},

/*
*    Other error constants.
*/

	{WSAHOST_NOT_FOUND, "WSAHOST_NOT_FOUND - Host not found"},
	{WSATRY_AGAIN, "WSATRY_AGAIN - Host not found or SERVERFAIL"},
	{WSANO_RECOVERY, "WSANO_RECOVERY - Non-recoverable error"},
	{WSANO_DATA, "WSANO_DATA - (or WSANO_ADDRESS) - No data record of requested type"},
	{-1, NULL}
};

#ifdef _DEBUG
void            WinPrint(char *str, ...);
#else
void            WinPrint(char *str, ...);
#endif

char           *WINS_ErrorMessage(int error)
{
	int             search = 0;

	if(!error)
		return "No error occurred";

	for(search = 0; errlist[search].errstr; search++)
	{
		if(error == errlist[search].errnum)
			return errlist[search].errstr;
	}							//end for

	return "Unknown error";
}								//end of the function WINS_ErrorMessage

int WINS_Init(void)
{
	int             i;
	struct hostent *local;
	char            buff[MAXHOSTNAMELEN];
	struct sockaddr_s addr;
	char           *p;
	int             r;
	WORD            wVersionRequested;

	wVersionRequested = MAKEWORD(1, 1);

	r = WSAStartup(wVersionRequested, &winsockdata);

	if(r)
	{
		WinPrint("Winsock initialization failed.\n");
		return -1;
	}

	/*
	   i = COM_CheckParm ("-udpport");
	   if (i == 0) */
	net_hostport = DEFAULTnet_hostport;
	/*
	   else if (i < com_argc-1)
	   net_hostport = Q_atoi (com_argv[i+1]);
	   else
	   Sys_Error ("WINS_Init: you must specify a number after -udpport");
	 */

	// determine my name & address
	gethostname(buff, MAXHOSTNAMELEN);
	local = gethostbyname(buff);
	myAddr = *(int *)local->h_addr_list[0];

	// if the quake hostname isn't set, set it to the machine name
//  if (Q_strcmp(hostname.string, "UNNAMED") == 0)
	{
		// see if it's a text IP address (well, close enough)
		for(p = buff; *p; p++)
			if((*p < '0' || *p > '9') && *p != '.')
				break;

		// if it is a real name, strip off the domain; we only want the host
		if(*p)
		{
			for(i = 0; i < 15; i++)
				if(buff[i] == '.')
					break;
			buff[i] = 0;
		}
//      Cvar_Set ("hostname", buff);
	}

	if((net_controlsocket = WINS_OpenSocket(0)) == -1)
		WinError("WINS_Init: Unable to open control socket\n");

	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons((u_short) net_hostport);

	WINS_GetSocketAddr(net_controlsocket, &addr);
	strcpy(my_tcpip_address, WINS_AddrToString(&addr));
	p = strrchr(my_tcpip_address, ':');
	if(p)
		*p = 0;
	WinPrint("Winsock Initialized\n");

	return net_controlsocket;
}								//end of the function WINS_Init

char           *WINS_MyAddress(void)
{
	return my_tcpip_address;
}								//end of the function WINS_MyAddress

void WINS_Shutdown(void)
{
	//WINS_Listen(0);
	WINS_CloseSocket(net_controlsocket);
	WSACleanup();
	//
	//WinPrint("Winsock Shutdown\n");
}								//end of the function WINS_Shutdown

/*
void WINS_Listen(int state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = WINS_OpenSocket (net_hostport)) == -1)
			WinError ("WINS_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	WINS_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
} //end of the function WINS_Listen*/

int WINS_OpenSocket(int port)
{
	int             newsocket;
	struct sockaddr_in address;
	u_long          _true = 1;

	if((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		WinPrint("WINS_OpenSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if

	if(ioctlsocket(newsocket, FIONBIO, &_true) == -1)
	{
		WinPrint("WINS_OpenSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		closesocket(newsocket);
		return -1;
	}							//end if

	memset((char *)&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short) port);
	if(bind(newsocket, (void *)&address, sizeof(address)) == -1)
	{
		WinPrint("WINS_OpenSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		closesocket(newsocket);
		return -1;
	}							//end if

	return newsocket;
}								//end of the function WINS_OpenSocket

int WINS_OpenReliableSocket(int port)
{
	int             newsocket;
	struct sockaddr_in address;
	BOOL            _true = 0xFFFFFFFF;

	//IPPROTO_TCP
	//
	if((newsocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		WinPrint("WINS_OpenReliableSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if

	memset((char *)&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons((u_short) port);
	if(bind(newsocket, (void *)&address, sizeof(address)) == -1)
	{
		WinPrint("WINS_OpenReliableSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		closesocket(newsocket);
		return -1;
	}							//end if

	//
	if(setsockopt(newsocket, IPPROTO_TCP, TCP_NODELAY, (void *)&_true, sizeof(int)) == SOCKET_ERROR)
	{
		WinPrint("WINS_OpenReliableSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		WinPrint("setsockopt error\n");
	}							//end if

	return newsocket;
}								//end of the function WINS_OpenReliableSocket

int WINS_Listen(int socket)
{
	u_long          _true = 1;

	if(ioctlsocket(socket, FIONBIO, &_true) == -1)
	{
		WinPrint("WINS_Listen: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	if(listen(socket, SOMAXCONN) == SOCKET_ERROR)
	{
		WinPrint("WINS_Listen: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	return 0;
}								//end of the function WINS_Listen

int WINS_Accept(int socket, struct sockaddr_s *addr)
{
	int             addrlen = sizeof(struct sockaddr_s);
	int             newsocket;
	BOOL            _true = 1;

	newsocket = accept(socket, (struct sockaddr *)addr, &addrlen);
	if(newsocket == INVALID_SOCKET)
	{
		if(WSAGetLastError() == WSAEWOULDBLOCK)
			return -1;
		WinPrint("WINS_Accept: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	//
	if(setsockopt(newsocket, IPPROTO_TCP, TCP_NODELAY, (void *)&_true, sizeof(int)) == SOCKET_ERROR)
	{
		WinPrint("WINS_Accept: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		WinPrint("setsockopt error\n");
	}							//end if
	return newsocket;
}								//end of the function WINS_Accept

int WINS_CloseSocket(int socket)
{
	/*
	   if (socket == net_broadcastsocket)
	   net_broadcastsocket = 0;
	 */
//  shutdown(socket, SD_SEND);

	if(closesocket(socket) == SOCKET_ERROR)
	{
		WinPrint("WINS_CloseSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return SOCKET_ERROR;
	}							//end if
	return 0;
}								//end of the function WINS_CloseSocket

static int PartialIPAddress(char *in, struct sockaddr_s *hostaddr)
{
	char            buff[256];
	char           *b;
	int             addr;
	int             num;
	int             mask;

	buff[0] = '.';
	b = buff;
	strcpy(buff + 1, in);
	if(buff[1] == '.')
		b++;

	addr = 0;
	mask = -1;
	while(*b == '.')
	{
		num = 0;
		if(*++b < '0' || *b > '9')
			return -1;
		while(!(*b < '0' || *b > '9'))
			num = num * 10 + *(b++) - '0';
		mask <<= 8;
		addr = (addr << 8) + num;
	}

	hostaddr->sa_family = AF_INET;
	((struct sockaddr_in *)hostaddr)->sin_port = htons((u_short) net_hostport);
	((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (myAddr & htonl(mask)) | htonl(addr);

	return 0;
}								//end of the function PartialIPAddress

int WINS_Connect(int socket, struct sockaddr_s *addr)
{
	int             ret;
	u_long          _true2 = 0xFFFFFFFF;

	ret = connect(socket, (struct sockaddr *)addr, sizeof(struct sockaddr_s));
	if(ret == SOCKET_ERROR)
	{
		WinPrint("WINS_Connect: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	if(ioctlsocket(socket, FIONBIO, &_true2) == -1)
	{
		WinPrint("WINS_Connect: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	return 0;
}								//end of the function WINS_Connect

int WINS_CheckNewConnections(void)
{
	char            buf[4];

	if(net_acceptsocket == -1)
		return -1;

	if(recvfrom(net_acceptsocket, buf, 4, MSG_PEEK, NULL, NULL) > 0)
		return net_acceptsocket;
	return -1;
}								//end of the function WINS_CheckNewConnections

//===========================================================================
// returns the number of bytes read
// 0 if no bytes available
// -1 on failure
//===========================================================================
int WINS_Read(int socket, byte * buf, int len, struct sockaddr_s *addr)
{
	int             addrlen = sizeof(struct sockaddr_s);
	int             ret, errno;

	if(addr)
	{
		ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)addr, &addrlen);
		if(ret == -1)
		{
			errno = WSAGetLastError();

			if(errno == WSAEWOULDBLOCK || errno == WSAECONNREFUSED)
				return 0;
		}						//end if
	}							//end if
	else
	{
		ret = recv(socket, buf, len, 0);
		if(ret == SOCKET_ERROR)
		{
			errno = WSAGetLastError();

			if(errno == WSAEWOULDBLOCK || errno == WSAECONNREFUSED)
				return 0;
		}						//end if
	}							//end else
	if(ret == SOCKET_ERROR)
	{
		WinPrint("WINS_Read: %s\n", WINS_ErrorMessage(WSAGetLastError()));
	}							//end if
	return ret;
}								//end of the function WINS_Read

int WINS_MakeSocketBroadcastCapable(int socket)
{
	int             i = 1;

	// make this socket broadcast capable
	if(setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}								//end of the function WINS_MakeSocketBroadcastCapable

int WINS_Broadcast(int socket, byte * buf, int len)
{
	int             ret;

	if(socket != net_broadcastsocket)
	{
		if(net_broadcastsocket != 0)
			WinError("Attempted to use multiple broadcasts sockets\n");
		ret = WINS_MakeSocketBroadcastCapable(socket);
		if(ret == -1)
		{
			WinPrint("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return WINS_Write(socket, buf, len, &broadcastaddr);
}								//end of the function WINS_Broadcast

//===========================================================================
// returns qtrue on success or qfalse on failure
//===========================================================================
int WINS_Write(int socket, byte * buf, int len, struct sockaddr_s *addr)
{
	int             ret, written;

	if(addr)
	{
		written = 0;
		while(written < len)
		{
			ret = sendto(socket, &buf[written], len - written, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_s));
			if(ret == SOCKET_ERROR)
			{
				if(WSAGetLastError() != WSAEWOULDBLOCK)
					return qfalse;
				Sleep(1000);
			}					//end if
			else
			{
				written += ret;
			}
		}
	}							//end if
	else
	{
		written = 0;
		while(written < len)
		{
			ret = send(socket, buf, len, 0);
			if(ret == SOCKET_ERROR)
			{
				if(WSAGetLastError() != WSAEWOULDBLOCK)
					return qfalse;
				Sleep(1000);
			}					//end if
			else
			{
				written += ret;
			}
		}
	}							//end else
	if(ret == SOCKET_ERROR)
	{
		WinPrint("WINS_Write: %s\n", WINS_ErrorMessage(WSAGetLastError()));
	}							//end if
	return (ret == len);
}								//end of the function WINS_Write

char           *WINS_AddrToString(struct sockaddr_s *addr)
{
	static char     buffer[22];
	int             haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff,
			ntohs(((struct sockaddr_in *)addr)->sin_port));
	return buffer;
}								//end of the function WINS_AddrToString

int WINS_StringToAddr(char *string, struct sockaddr_s *addr)
{
	int             ha1, ha2, ha3, ha4, hp;
	int             ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons((u_short) hp);
	return 0;
}								//end of the function WINS_StringToAddr

int WINS_GetSocketAddr(int socket, struct sockaddr_s *addr)
{
	int             addrlen = sizeof(struct sockaddr_s);
	unsigned int    a;

	memset(addr, 0, sizeof(struct sockaddr_s));
	getsockname(socket, (struct sockaddr *)addr, &addrlen);
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	if(a == 0 || a == inet_addr("127.0.0.1"))
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

	return 0;
}								//end of the function WINS_GetSocketAddr

int WINS_GetNameFromAddr(struct sockaddr_s *addr, char *name)
{
	struct hostent *hostentry;

	hostentry = gethostbyaddr((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if(hostentry)
	{
		strncpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	strcpy(name, WINS_AddrToString(addr));
	return 0;
}								//end of the function WINS_GetNameFromAddr

int WINS_GetAddrFromName(char *name, struct sockaddr_s *addr)
{
	struct hostent *hostentry;

	if(name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress(name, addr);

	hostentry = gethostbyname(name);
	if(!hostentry)
		return -1;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_port = htons((u_short) net_hostport);
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];

	return 0;
}								//end of the function WINS_GetAddrFromName

int WINS_AddrCompare(struct sockaddr_s *addr1, struct sockaddr_s *addr2)
{
	if(addr1->sa_family != addr2->sa_family)
		return -1;

	if(((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
		return -1;

	if(((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port)
		return 1;

	return 0;
}								//end of the function WINS_AddrCompare

int WINS_GetSocketPort(struct sockaddr_s *addr)
{
	return ntohs(((struct sockaddr_in *)addr)->sin_port);
}								//end of the function WINS_GetSocketPort

int WINS_SetSocketPort(struct sockaddr_s *addr, int port)
{
	((struct sockaddr_in *)addr)->sin_port = htons((u_short) port);
	return 0;
}

#endif





/*
===================================================================

UNIX

===================================================================
*/
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__sun) || defined(__SunOS__)
#if defined(__sun) || defined(__SunOS__)
#include <sys/filio.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

#define WinError WinPrint

#define qtrue	1
#define qfalse	0

#define ioctlsocket ioctl
#define closesocket close

int WSAGetLastError()
{
	return errno;
}

/*
typedef struct tag_error_struct
{
    int     errnum;
    LPSTR   errstr;
} ERROR_STRUCT;
*/

typedef struct tag_error_struct
{
	int             errnum;
	const char     *errstr;
} ERROR_STRUCT;

#define	NET_NAMELEN			64

static char     my_tcpip_address[NET_NAMELEN];

#define	DEFAULTnet_hostport	26000

#define MAXHOSTNAMELEN		256

static int      net_acceptsocket = -1;	// socket for fielding new connections
static int      net_controlsocket;
static int      net_hostport;	// udp port number for acceptsocket
static int      net_broadcastsocket = 0;

//static qboolean ifbcastinit = qfalse;
//static struct sockaddr_s broadcastaddr;
static struct sockaddr_s broadcastaddr;

static unsigned long myAddr;

ERROR_STRUCT    errlist[] = {
	{EACCES, "EACCES - The address is protected, user is not root"},
	{EAGAIN, "EAGAIN - Operation on non-blocking socket that cannot return immediatly"},
	{EBADF, "EBADF - sockfd is not a valid descriptor"},
	{EFAULT, "EFAULT - The parameter is not in a writable part of the user address space"},
	{EINVAL, "EINVAL - The socket is already bound to an address"},
	{ENOBUFS, "ENOBUFS - not enough memory"},
	{ENOMEM, "ENOMEM - not enough memory"},
	{ENOTCONN, "ENOTCONN - not connected"},
	{ENOTSOCK, "ENOTSOCK - Argument is file descriptor not a socket"},
	{EOPNOTSUPP, "ENOTSUPP - The referenced socket is not of type SOCK_STREAM"},
	{EPERM, "EPERM - Firewall rules forbid connection"},
	{-1, NULL}
};

char           *WINS_ErrorMessage(int error)
{
	int             search = 0;

	if(!error)
		return "No error occurred";

	for(search = 0; errlist[search].errstr; search++)
	{
		if(error == errlist[search].errnum)
			return (char *)errlist[search].errstr;
	}							//end for

	return "Unknown error";
}								//end of the function WINS_ErrorMessage

int WINS_Init(void)
{
	int             i;
	struct hostent *local;
	char            buff[MAXHOSTNAMELEN];
	struct sockaddr_s addr;
	char           *p;

/* 
 linux doesn't have anything to initialize for the net
 "Windows .. built for the internet .. the internet .. built with unix" 
 */
#if 0
	WORD            wVersionRequested;

	wVersionRequested = MAKEWORD(2, 2);

	r = WSAStartup(wVersionRequested, &winsockdata);

	if(r)
	{
		WinPrint("Winsock initialization failed.\n");
		return -1;
	}
#endif
	/*
	   i = COM_CheckParm ("-udpport");
	   if (i == 0) */
	net_hostport = DEFAULTnet_hostport;
	/*
	   else if (i < com_argc-1)
	   net_hostport = Q_atoi (com_argv[i+1]);
	   else
	   Sys_Error ("WINS_Init: you must specify a number after -udpport");
	 */

	// determine my name & address
	gethostname(buff, MAXHOSTNAMELEN);
	local = gethostbyname(buff);
	myAddr = *(int *)local->h_addr_list[0];

	// if the quake hostname isn't set, set it to the machine name
//  if (Q_strcmp(hostname.string, "UNNAMED") == 0)
	{
		// see if it's a text IP address (well, close enough)
		for(p = buff; *p; p++)
			if((*p < '0' || *p > '9') && *p != '.')
				break;

		// if it is a real name, strip off the domain; we only want the host
		if(*p)
		{
			for(i = 0; i < 15; i++)
				if(buff[i] == '.')
					break;
			buff[i] = 0;
		}
//      Cvar_Set ("hostname", buff);
	}

	//++timo WTF is that net_controlsocket? it's sole purpose is to retrieve the local IP?
	if((net_controlsocket = WINS_OpenSocket(0)) == SOCKET_ERROR)
		WinError("WINS_Init: Unable to open control socket\n");

	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons((u_short) net_hostport);

	WINS_GetSocketAddr(net_controlsocket, &addr);
	strcpy(my_tcpip_address, WINS_AddrToString(&addr));
	p = strrchr(my_tcpip_address, ':');
	if(p)
		*p = 0;
	WinPrint("Winsock Initialized\n");

	return net_controlsocket;
}								//end of the function WINS_Init

char           *WINS_MyAddress(void)
{
	return my_tcpip_address;
}								//end of the function WINS_MyAddress

void WINS_Shutdown(void)
{
	//WINS_Listen(0);
	WINS_CloseSocket(net_controlsocket);
//  WSACleanup();
	//
	//WinPrint("Winsock Shutdown\n");
}								//end of the function WINS_Shutdown

/*
void WINS_Listen(int state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = WINS_OpenSocket (net_hostport)) == -1)
			WinError ("WINS_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	WINS_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
} //end of the function WINS_Listen*/

int WINS_OpenSocket(int port)
{
	int             newsocket;
	struct sockaddr_in address;
	u_long          _true = 1;

	if((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		WinPrint("WINS_OpenSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if

	if(ioctlsocket(newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		WinPrint("WINS_OpenSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		closesocket(newsocket);
		return -1;
	}							//end if

	memset((char *)&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short) port);
	if(bind(newsocket, (void *)&address, sizeof(address)) == -1)
	{
		WinPrint("WINS_OpenSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		closesocket(newsocket);
		return -1;
	}							//end if

	return newsocket;
}								//end of the function WINS_OpenSocket

int WINS_OpenReliableSocket(int port)
{
	int             newsocket;
	struct sockaddr_in address;
	qboolean        _true = 0xFFFFFFFF;

	//IPPROTO_TCP
	//
	if((newsocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		WinPrint("WINS_OpenReliableSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if

	memset((char *)&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons((u_short) port);
	if(bind(newsocket, (void *)&address, sizeof(address)) == -1)
	{
		WinPrint("WINS_OpenReliableSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		closesocket(newsocket);
		return -1;
	}							//end if

	//
	if(setsockopt(newsocket, IPPROTO_TCP, TCP_NODELAY, (void *)&_true, sizeof(int)) == -1)
	{
		WinPrint("WINS_OpenReliableSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		WinPrint("setsockopt error\n");
	}							//end if

	return newsocket;
}								//end of the function WINS_OpenReliableSocket

int WINS_Listen(int socket)
{
	u_long          _true = 1;

	if(ioctlsocket(socket, FIONBIO, &_true) == -1)
	{
		WinPrint("WINS_Listen: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	if(listen(socket, SOMAXCONN) == SOCKET_ERROR)
	{
		WinPrint("WINS_Listen: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	return 0;
}								//end of the function WINS_Listen

int WINS_Accept(int socket, struct sockaddr_s *addr)
{
	int             addrlen = sizeof(struct sockaddr_s);
	int             newsocket;
	qboolean        _true = 1;

	newsocket = accept(socket, (struct sockaddr *)addr, &addrlen);
	if(newsocket == INVALID_SOCKET)
	{
		if(errno == EAGAIN)
			return -1;
		WinPrint("WINS_Accept: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	//
	if(setsockopt(newsocket, IPPROTO_TCP, TCP_NODELAY, (void *)&_true, sizeof(int)) == SOCKET_ERROR)
	{
		WinPrint("WINS_Accept: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		WinPrint("setsockopt error\n");
	}							//end if
	return newsocket;
}								//end of the function WINS_Accept

int WINS_CloseSocket(int socket)
{
	/*
	   if (socket == net_broadcastsocket)
	   net_broadcastsocket = 0;
	 */
//  shutdown(socket, SD_SEND);

	if(closesocket(socket) == SOCKET_ERROR)
	{
		WinPrint("WINS_CloseSocket: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return SOCKET_ERROR;
	}							//end if
	return 0;
}								//end of the function WINS_CloseSocket

static int PartialIPAddress(char *in, struct sockaddr_s *hostaddr)
{
	char            buff[256];
	char           *b;
	int             addr;
	int             num;
	int             mask;

	buff[0] = '.';
	b = buff;
	strcpy(buff + 1, in);
	if(buff[1] == '.')
		b++;

	addr = 0;
	mask = -1;
	while(*b == '.')
	{
		num = 0;
		if(*++b < '0' || *b > '9')
			return -1;
		while(!(*b < '0' || *b > '9'))
			num = num * 10 + *(b++) - '0';
		mask <<= 8;
		addr = (addr << 8) + num;
	}

	hostaddr->sa_family = AF_INET;
	((struct sockaddr_in *)hostaddr)->sin_port = htons((u_short) net_hostport);
	((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (myAddr & htonl(mask)) | htonl(addr);

	return 0;
}								//end of the function PartialIPAddress

int WINS_Connect(int socket, struct sockaddr_s *addr)
{
	int             ret;
	u_long          _true2 = 0xFFFFFFFF;

	ret = connect(socket, (struct sockaddr *)addr, sizeof(struct sockaddr_s));
	if(ret == SOCKET_ERROR)
	{
		WinPrint("WINS_Connect: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	if(ioctlsocket(socket, FIONBIO, &_true2) == -1)
	{
		WinPrint("WINS_Connect: %s\n", WINS_ErrorMessage(WSAGetLastError()));
		return -1;
	}							//end if
	return 0;
}								//end of the function WINS_Connect

int WINS_CheckNewConnections(void)
{
	char            buf[4];

	if(net_acceptsocket == -1)
		return -1;

	if(recvfrom(net_acceptsocket, buf, 4, MSG_PEEK, NULL, NULL) > 0)
		return net_acceptsocket;
	return -1;
}								//end of the function WINS_CheckNewConnections

int WINS_Read(int socket, byte * buf, int len, struct sockaddr_s *addr)
{
	int             addrlen = sizeof(struct sockaddr_s);
	int             ret;

	if(addr)
	{
		ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)addr, &addrlen);
		if(ret == -1)
		{
//          errno = WSAGetLastError();

			if(errno == EAGAIN || errno == ENOTCONN)
				return 0;
		}						//end if
	}							//end if
	else
	{
		ret = recv(socket, buf, len, 0);
		// if there's no data on the socket ret == -1 and errno == EAGAIN
		// MSDN states that if ret == 0 the socket has been closed
		// man recv doesn't say anything
		if(ret == 0)
			return -1;
		if(ret == SOCKET_ERROR)
		{
//          errno = WSAGetLastError();

			if(errno == EAGAIN || errno == ENOTCONN)
				return 0;
		}						//end if
	}							//end else
	if(ret == SOCKET_ERROR)
	{
		WinPrint("WINS_Read: %s\n", WINS_ErrorMessage(WSAGetLastError()));
	}							//end if
	return ret;
}								//end of the function WINS_Read

int WINS_MakeSocketBroadcastCapable(int socket)
{
	int             i = 1;

	// make this socket broadcast capable
	if(setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}								//end of the function WINS_MakeSocketBroadcastCapable

int WINS_Broadcast(int socket, byte * buf, int len)
{
	int             ret;

	if(socket != net_broadcastsocket)
	{
		if(net_broadcastsocket != 0)
			WinError("Attempted to use multiple broadcasts sockets\n");
		ret = WINS_MakeSocketBroadcastCapable(socket);
		if(ret == -1)
		{
			WinPrint("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return WINS_Write(socket, buf, len, &broadcastaddr);
}								//end of the function WINS_Broadcast

int WINS_Write(int socket, byte * buf, int len, struct sockaddr_s *addr)
{
	int             ret, written;

	if(addr)
	{
		written = 0;
		while(written < len)
		{
			ret = sendto(socket, &buf[written], len - written, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_s));
			if(ret == SOCKET_ERROR)
			{
				if(WSAGetLastError() != EAGAIN)
					return qfalse;
				//++timo FIXME: what is this used for?
//              Sleep(1000);
			}					//end if
			else
			{
				written += ret;
			}
		}
	}							//end if
	else
	{
		written = 0;
		while(written < len)
		{
			ret = send(socket, buf, len, 0);
			if(ret == SOCKET_ERROR)
			{
				if(WSAGetLastError() != EAGAIN)
					return qfalse;
				//++timo FIXME: what is this used for?
//              Sleep(1000);
			}					//end if
			else
			{
				written += ret;
			}
		}
	}							//end else
	if(ret == SOCKET_ERROR)
	{
		WinPrint("WINS_Write: %s\n", WINS_ErrorMessage(WSAGetLastError()));
	}							//end if
	return (ret == len);
}								//end of the function WINS_Write

char           *WINS_AddrToString(struct sockaddr_s *addr)
{
	static char     buffer[22];
	int             haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff,
			ntohs(((struct sockaddr_in *)addr)->sin_port));
	return buffer;
}								//end of the function WINS_AddrToString

int WINS_StringToAddr(char *string, struct sockaddr_s *addr)
{
	int             ha1, ha2, ha3, ha4, hp;
	int             ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons((u_short) hp);
	return 0;
}								//end of the function WINS_StringToAddr

int WINS_GetSocketAddr(int socket, struct sockaddr_s *addr)
{
	int             addrlen = sizeof(struct sockaddr_s);
	unsigned int    a;

	memset(addr, 0, sizeof(struct sockaddr_s));
	getsockname(socket, (struct sockaddr *)addr, &addrlen);
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	if(a == 0 || a == inet_addr("127.0.0.1"))
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

	return 0;
}								//end of the function WINS_GetSocketAddr

int WINS_GetNameFromAddr(struct sockaddr_s *addr, char *name)
{
	struct hostent *hostentry;

	hostentry = gethostbyaddr((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if(hostentry)
	{
		strncpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	strcpy(name, WINS_AddrToString(addr));
	return 0;
}								//end of the function WINS_GetNameFromAddr

int WINS_GetAddrFromName(char *name, struct sockaddr_s *addr)
{
	struct hostent *hostentry;

	if(name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress(name, addr);

	hostentry = gethostbyname(name);
	if(!hostentry)
		return -1;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_port = htons((u_short) net_hostport);
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];

	return 0;
}								//end of the function WINS_GetAddrFromName

int WINS_AddrCompare(struct sockaddr_s *addr1, struct sockaddr_s *addr2)
{
	if(addr1->sa_family != addr2->sa_family)
		return -1;

	if(((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
		return -1;

	if(((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port)
		return 1;

	return 0;
}								//end of the function WINS_AddrCompare

int WINS_GetSocketPort(struct sockaddr_s *addr)
{
	return ntohs(((struct sockaddr_in *)addr)->sin_port);
}								//end of the function WINS_GetSocketPort

int WINS_SetSocketPort(struct sockaddr_s *addr, int port)
{
	((struct sockaddr_in *)addr)->sin_port = htons((u_short) port);
	return 0;
}								//end of the function WINS_SetSocketPort

#endif							// UNIX
