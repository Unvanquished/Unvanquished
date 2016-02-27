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
#ifndef COMMON_NETWORK_H
#define COMMON_NETWORK_H

namespace Net {

inline const std::string& OOBHeader()
{
	static const std::string header ="\xff\xff\xff\xff";
	return header;
}

template<class... Args>
void OutOfBandPrint( netsrc_t net_socket, const netadr_t& adr, Str::StringRef format, Args&&... args )
{
	std::string message = OOBHeader() + Str::Format( format, std::forward<Args>(args)... );
	NET_SendPacket( net_socket, message.size(), message.c_str(), adr );
}

/*
 * Sends huffman-compressed data
 */
void OutOfBandData( netsrc_t sock, const netadr_t& adr, byte* data, std::size_t len );

/*
 * Converts an address to its string representation
 */
std::string AddressToString( const netadr_t& address, bool with_port = false );

/*
 * Returns port, port4 or port6 depending on the address type
 */
unsigned short GetPort(const netadr_t& address);


} // namespace Net

#endif // COMMON_NETWORK_H
