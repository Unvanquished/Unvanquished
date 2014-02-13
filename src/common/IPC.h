/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef COMMON_IPC_H_
#define COMMON_IPC_H_

#include <type_traits>
#include <tuple>
#include <vector>
#include <string>
#include "String.h"
#include <limits>
#include "Util.h"
#include "../libs/nacl/nacl.h"
#include "../engine/qcommon/q_shared.h"
#include "../engine/qcommon/qcommon.h"

namespace IPC {

// Maximum number of bytes that can be sent in a message
const size_t MSG_MAX_BYTES = 128 << 10;

// Maximum number of handles that can be sent in a message
const size_t MSG_MAX_HANDLES = 8;

// Operating system handle type
#ifdef _WIN32
// HANDLE is defined as void* in windows.h
// Note that the NaCl code uses -1 instead of 0 to represent invalid handles
typedef void* OSHandleType;
const OSHandleType INVALID_HANDLE = reinterpret_cast<void*>(-1);
#else
typedef int OSHandleType;
const OSHandleType INVALID_HANDLE = -1;
#endif

// IPC descriptor which can be sent over a socket. You should treat this as an
// opaque type and not access any of the fields directly.
#ifdef __native_client__
typedef int Desc;
#else
struct Desc {
	OSHandleType handle;
	int type;
	union {
		uint64_t size;
		int32_t flags;
	};
};
#endif
void CloseDesc(const Desc& desc);

// Conversion between Desc and handles
enum FileOpenMode {
	MODE_READ,
	MODE_WRITE,
	MODE_RW,
	MODE_WRITE_APPEND,
	MODE_RW_APPEND
};
Desc DescFromFile(OSHandleType handle, FileOpenMode mode);
OSHandleType DescToHandle(const Desc& desc);

// Message-based socket through which data and handles can be passed.
class Reader;
class Writer;
class Socket {
public:
	Socket()
		: handle(INVALID_HANDLE) {}
	Socket(Socket&& other) NOEXCEPT
		: handle(other.handle)
	{
		other.handle = INVALID_HANDLE;
	}
	Socket& operator=(Socket&& other) NOEXCEPT
	{
		std::swap(handle, other.handle);
		return *this;
	}
	~Socket()
	{
		Close();
	}
	explicit operator bool() const
	{
		return handle != INVALID_HANDLE;
	}

	void Close();

	Desc GetDesc() const;
	static Socket FromDesc(const Desc& desc);

	void SendMsg(const Writer& writer) const;
	Reader RecvMsg() const;

	static std::pair<Socket, Socket> CreatePair();

private:
	OSHandleType handle;
};

// Shared memory area, can be sent over a socket
class SharedMemory {
public:
	SharedMemory()
		: handle(INVALID_HANDLE) {}
	SharedMemory(SharedMemory&& other) NOEXCEPT
		: handle(other.handle), base(other.base), size(other.size)
	{
		other.handle = INVALID_HANDLE;
	}
	SharedMemory& operator=(SharedMemory&& other) NOEXCEPT
	{
		std::swap(handle, other.handle);
		std::swap(base, other.base);
		std::swap(size, other.size);
		return *this;
	}
	~SharedMemory()
	{
		Close();
	}
	explicit operator bool() const
	{
		return handle != INVALID_HANDLE;
	}

	void Close();

	Desc GetDesc() const;
	static SharedMemory FromDesc(const Desc& desc);

	static SharedMemory Create(size_t size);

	void* GetBase() const
	{
		return base;
	}
	size_t GetSize() const
	{
		return size;
	}

private:
	OSHandleType handle;
	void* base;
	size_t size;
};

// Base type for serialization traits.
template<typename T, typename = void> struct SerializeTraits {};

// Class to generate messages
class Writer {
public:
	void WriteData(const void* p, size_t len)
	{
		data.insert(data.end(), static_cast<const char*>(p), static_cast<const char*>(p) + len);
	}
	void WriteSize(size_t size)
	{
		if (size > std::numeric_limits<uint32_t>::max())
			Com_Error(ERR_DROP, "IPC: Size out of range in message");
		Write<uint32_t>(size);
	}
	template<typename T, typename Arg> void Write(Arg&& value)
	{
		SerializeTraits<T>::Write(*this, std::forward<Arg>(value));
	}
	void WriteHandle(const Desc& h)
	{
		handles.push_back(h);
	}

	const std::vector<char>& GetData() const
	{
		return data;
	}
	const std::vector<Desc>& GetHandles() const
	{
		return handles;
	}

private:
	std::vector<char> data;
	std::vector<Desc> handles;
};

// Class to read messages
class Reader {
public:
	Reader()
		: pos(0), handles_pos(0) {}
	Reader(Reader&& other) NOEXCEPT
		: data(std::move(other.data)), handles(std::move(other.handles)), pos(other.pos), handles_pos(other.handles_pos) {}
	Reader& operator=(Reader&& other) NOEXCEPT
	{
		std::swap(data, other.data);
		std::swap(handles, other.handles);
		std::swap(pos, other.pos);
		std::swap(handles_pos, other.handles_pos);
		return *this;
	}
	~Reader()
	{
		// Close any handles that weren't read
		for (size_t i = handles_pos; i < handles.size(); i++)
			CloseDesc(handles[i]);
	}

	void ReadData(void* p, size_t len)
	{
		if (pos + len <= data.size()) {
			memcpy(p, &data[pos], len);
			pos += len;
		} else
			Com_Error(ERR_DROP, "IPC: Unexpected end of message");
	}
	template<typename T> size_t ReadSize()
	{
		// Check for multiplication overflows when reading a size
		uint32_t size;
		ReadData(&size, sizeof(uint32_t));
		if (size > std::numeric_limits<uint32_t>::max() / sizeof(T))
			Com_Error(ERR_DROP, "IPC: Size out of range in message");
		return size;
	}
	const void* ReadInline(size_t len)
	{
		if (pos + len <= data.size()) {
			const void* out = &data[pos];
			pos += len;
			return out;
		} else
			Com_Error(ERR_DROP, "IPC: Unexpected end of message");
	}
	template<typename T> decltype(SerializeTraits<T>::Read(std::declval<Reader&>())) Read()
	{
		return SerializeTraits<T>::Read(*this);
	}
	Desc ReadHandle()
	{
		if (handles_pos <= handles.size())
			return handles[handles_pos++];
		else
			Com_Error(ERR_DROP, "IPC: Unexpected end of message");
	}

	std::vector<char>& GetData()
	{
		return data;
	}
	std::vector<Desc>& GetHandles()
	{
		return handles;
	}

private:
	std::vector<char> data;
	std::vector<Desc> handles;
	size_t pos;
	size_t handles_pos;
};

// Simple implementation for POD types
template<typename T>
struct SerializeTraits<T, typename std::enable_if<std::is_pod<T>::value>::type> {
	static void Write(Writer& stream, const T& value)
	{
		stream.WriteData(std::addressof(value), sizeof(value));
	}
	static T Read(Reader& stream)
	{
		T value;
		stream.ReadData(std::addressof(value), sizeof(value));
		return value;
	}
};

// std::vector, with a specialization for POD types
template<typename T>
struct SerializeTraits<std::vector<T>, typename std::enable_if<std::is_pod<T>::value>::type> {
	static void Write(Writer& stream, const std::vector<T>& value)
	{
		stream.WriteSize(value.size());
		stream.WriteData(value.data(), value.size() * sizeof(T));
	}
	static std::vector<T> Read(Reader& stream)
	{
		std::vector<T> value;
		value.resize(stream.ReadSize<T>());
		stream.ReadData(value.data(), value.size() * sizeof(T));
		return value;
	}
};
template<typename T>
struct SerializeTraits<std::vector<T>, typename std::enable_if<!std::is_pod<T>::value>::type> {
	static void Write(Writer& stream, const std::vector<T>& value)
	{
		stream.WriteSize(value.size());
		for (const T& x: value)
			SerializeTraits<T>::Write(stream, x);
	}
	static std::vector<T> Read(Reader& stream)
	{
		std::vector<T> value;
		value.resize(stream.ReadSize<T>());
		for (T& x: value)
			x = SerializeTraits<T>::Read(stream);
		return value;
	}
};

// std::string, returns a StringRef into the stream buffer
template<> struct SerializeTraits<std::string> {
	static void Write(Writer& stream, Str::StringRef value)
	{
		stream.WriteSize(value.size());
		stream.WriteData(value.c_str(), value.size() + 1);
	}
	static Str::StringRef Read(Reader& stream)
	{
		size_t size = stream.ReadSize<char>();
		const char* p = static_cast<const char*>(stream.ReadInline(size + 1));
		if (p[size] != '\0')
			Com_Error(ERR_DROP, "IPC: String in message is not NUL-terminated");
		return p;
	}
};

// IPC message, which automatically serializes and deserializes objects
template<uint32_t Id, typename... T> class Message: public Writer {
	template<size_t Index, typename Tuple> static void FillTuple(Tuple&, Reader&) {}
	template<size_t Index, typename Type0, typename... Types, typename Tuple> static void FillTuple(Tuple& tuple, Reader& stream)
	{
		std::get<Index>(tuple) = stream.Read<Type0>();
		FillTuple<Index + 1, Types...>(tuple, stream);
	}

	template<size_t Index> void SerializeImpl() {}
	template<size_t Index, typename Arg0, typename... Args> void SerializeImpl(Arg0&& arg0, Args&&... args)
	{
		Write<typename std::tuple_element<Index, std::tuple<T...>>::type>(std::forward<Arg0>(arg0));
		SerializeImpl<Index + 1>(std::forward<Args>(args)...);
	}

public:
	// Construct a message for sending. The message object contains a Writer which
	// has the actual message data and can be passed to SendMsg().
	template<typename... Args> Message(Args&&... args)
	{
		static_assert(sizeof...(Args) == sizeof...(T), "Incorrect number of arguments for IPC::Message::Serialize");
		Write<uint32_t>(Id);
		SerializeImpl<0>(std::forward<Args>(args)...);
	}

	// Deserialize a message and call the given function with the message arguments
	template<typename Func> static void Deserialize(Reader& stream, Func&& func)
	{
		std::tuple<decltype(stream.Read<T>())...> tuple;
		FillTuple<0, T...>(tuple, stream);
		Util::apply(std::forward<Func>(func), std::move(tuple));
	}
};

// Message ID to indicate an RPC return
const uint32_t MSGID_RETURN = 0xffffffffu;

// Helper function to perform a synchronous RPC. This involves sending a message
// and then processing all incoming messages until a MSGID_RETURN is recieved.
template<typename Func> Reader DoRPC(const Socket& socket, const Writer& writer, Func&& syscallHandler)
{
	socket.SendMsg(writer);

	while (true) {
		Reader reader = socket.RecvMsg();
		uint32_t id = reader.Read<uint32_t>();
		if (id == MSGID_RETURN)
			return reader;
		syscallHandler(id, std::move(reader));
	}
}

} // namespace IPC

#endif // COMMON_IPC_H_
