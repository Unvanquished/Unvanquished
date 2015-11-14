#ifndef COMMON_NETWORK_H
#define COMMON_NETWORK_H

namespace Net {

inline const std::string& OOBHeader()
{
	static std::string header ="\xff\xff\xff\xff";
	return header;
}

template<class... Args>
void OutOfBandPrint( netsrc_t net_socket, netadr_t adr, Str::StringRef format, Args&&... args )
{
	std::string message = OOBHeader() + Str::Format( format, std::forward<Args>(args)... );
	NET_SendPacket( net_socket, message.size(), message.c_str(), adr );
}

} // namespace Net

#endif // COMMON_NETWORK_H
