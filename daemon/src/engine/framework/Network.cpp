/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/
#include "common/Common.h"
#include "engine/qcommon/qcommon.h"
#include "Network.h"

#ifdef _WIN32
#       include <winsock2.h>
#else
#       include <sys/socket.h>
#       include <netinet/in.h>
#endif

// Helper functions defined in net_ip.cpp
void Sys_SockaddrToString( char *dest, int destlen, struct sockaddr *input );
void NetadrToSockadr( const netadr_t *a, struct sockaddr *s );

namespace Net {

void OutOfBandData( netsrc_t sock, const netadr_t& adr, byte *data, std::size_t len )
{
	if ( len == 0 )
	{
		return;
	}

	std::basic_string<byte> message;
	message.reserve(OOBHeader().size() + len);
	message.append(OOBHeader().begin(), OOBHeader().end());
	message.append(data, len);

	msg_t mbuf;
	mbuf.data = &message[0];
	mbuf.cursize = message.size();
	Huff_Compress( &mbuf, 12 );
	// send the datagram
	NET_SendPacket( sock, mbuf.cursize, mbuf.data, adr );
}

std::string AddressToString( const netadr_t& address, bool with_port )
{

    if ( address.type == netadrtype_t::NA_LOOPBACK )
    {
        return "loopback";
    }
    else if ( address.type == netadrtype_t::NA_BOT )
    {
        return "bot";
    }
    else if ( address.type == netadrtype_t::NA_IP ||
              address.type == netadrtype_t::NA_IP6 ||
              address.type == netadrtype_t::NA_IP_DUAL )
    {
        char s[ NET_ADDR_STR_MAX_LEN ];
        sockaddr_storage sadr = {0};
        NetadrToSockadr( &address, reinterpret_cast<sockaddr*>(&sadr) );
        Sys_SockaddrToString( s, sizeof(s), reinterpret_cast<sockaddr*>(&sadr) );

        std::string result = s;
        if ( with_port )
        {
            if ( NET_IS_IPv6( address.type ) )
            {
                result = '[' + result + ']';
            }
                result += ':' + std::to_string(ntohs(GetPort(address)));
        }
        return result;
    }

    return "";
}

unsigned short GetPort(const netadr_t& address)
{
    if ( address.type == netadrtype_t::NA_IP_DUAL )
    {
        if ( NET_IS_IPv4( address.type ) )
        {
            return address.port4;
        }
        else if ( NET_IS_IPv6( address.type ) )
        {
            return address.port6;
        }
    }
    return address.port;
}

} // namespace Net
