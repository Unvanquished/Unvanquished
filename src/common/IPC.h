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

// std::pair
template<typename T, typename U>
struct SerializeTraits<std::pair<T, U>> {
	static void Write(Writer& stream, const std::pair<T, U>& value)
	{
		SerializeTraits<T>::Write(stream, value.first);
		SerializeTraits<U>::Write(stream, value.second);
	}
	static std::pair<T, U> Read(Reader& stream)
	{
		std::pair<T, U> value;
		value.first = SerializeTraits<T>::Read(stream);
		value.second = SerializeTraits<U>::Read(stream);
		return value;
	}
};

// std::string
template<> struct SerializeTraits<std::string> {
	static void Write(Writer& stream, Str::StringRef value)
	{
		stream.WriteSize(value.size());
		stream.WriteData(value.c_str(), value.size());
	}
	static std::string Read(Reader& stream)
	{
		size_t size = stream.ReadSize<char>();
		const char* p = static_cast<const char*>(stream.ReadInline(size));
		return std::string(p, p + size);
	}
};

// Message ID to indicate an RPC return
const uint32_t ID_RETURN = 0xffffffffu;

// Combine a major and minor ID into a single number
#define IPC_ID(major, minor) ((((uint16_t)major) << 16) + ((uint16_t)minor))

// Asynchronous message which does not wait for a reply
template<uint32_t Id, typename... T> struct Message {
	static const uint32_t id = Id;
	typedef std::tuple<T...> Inputs;
};

// Synchronous message which waits for a reply. The reply can contain data.
template<typename InMsg, typename... Out> class SyncMessage {
	static const uint32_t id = InMsg::id;
	typedef typename InMsg::Inputs Inputs;
	typedef std::tuple<Out...> Outputs;
};

namespace detail {

// Serialize a list of types into a Writer (ignores extra trailing arguments)
template<typename... Args> void SerializeArgs(Writer&, std::tuple<>&&, Args&&...) {}
template<typename Type0, typename... Types, typename Arg0, typename... Args> void SerializeArgs(Writer& writer, std::tuple<Type0, Types...>&&, Arg0&& arg0, Args&&... args)
{
	writer.Write<Type0>(std::forward<Arg0>(arg0));
	SerializeArgs(writer, std::tuple<Types...>(), std::forward<Args>(args)...);
}

// Read a list of types into a tuple starting at the given index
template<size_t Index, typename Tuple> void FillTuple(Tuple&, Reader&) {}
template<size_t Index, typename Type0, typename... Types, typename Tuple> void FillTuple(Tuple& tuple, Reader& stream)
{
	std::get<Index>(tuple) = stream.Read<Type0>();
	FillTuple<Index + 1, Types...>(tuple, stream);
}

// Create a tuple of references from a tuple. The type of reference is the same as that with which the tuple is passed in.
template<size_t... Seq, typename Tuple> decltype(std::forward_as_tuple(std::get<Seq>(std::declval<Tuple>())...)) RefTuple(Tuple&& tuple, Util::seq<Seq...>)
{
	return std::forward_as_tuple(std::get<Seq>(std::forward<Tuple>(tuple))...);
}

// Implementations of SendMsg for Message and SyncMessage
template<typename Func, uint32_t Id, typename... MsgArgs, typename... Args> void SendMsg(const Socket& socket, Func&&, Message<Id, MsgArgs...>, Args&&... args)
{
	typedef Message<Id, MsgArgs...> Message;
	static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value, "Incorrect number of arguments for IPC::SendMsg");

	Writer writer;
	writer.Write<uint32_t>(Message::id);
	SerializeArgs(writer, typename Message::Inputs(), std::forward<Args>(args)...);
	socket.SendMsg(writer);
}
template<typename Func, typename MsgIn, typename... MsgOutArgs, typename... Args> void SendMsg(const Socket& socket, Func&& messageHandler, SyncMessage<MsgIn, MsgOutArgs...>, Args&&... args)
{
	typedef SyncMessage<MsgIn, MsgOutArgs...> Message;
	static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value + sizeof...(MsgOutArgs), "Incorrect number of arguments for IPC::SendMsg");

	Writer writer;
	writer.Write<uint32_t>(Message::id);
	SerializeArgs(writer, typename Message::Inputs(), std::forward<Args>(args)...);
	socket.SendMsg(writer);

	while (true) {
		Reader reader = socket.RecvMsg();
		uint32_t id = reader.Read<uint32_t>();
		if (id == ID_RETURN) {
			auto out = std::forward_as_tuple(std::forward<Args>(args)...);
			FillTuple<std::tuple_size<typename Message::Inputs>::value, MsgOutArgs...>(out, reader);
			return;
		}
		messageHandler(id, std::move(reader));
	}
}

// Implementations of HandleMsg for Message and SyncMessage
template<typename Func, uint32_t Id, typename... MsgArgs> void HandleMsg(const Socket&, Message<Id, MsgArgs...>, IPC::Reader reader, Func&& func)
{
	typedef Message<Id, MsgArgs...> Message;

	typename Message::Inputs inputs;
	FillTuple<0>(inputs, reader);
	Util::apply(std::forward<Func>(func), std::move(inputs));
}
template<typename Func, typename MsgIn, typename... MsgOutArgs> void HandleMsg(const Socket& socket, SyncMessage<MsgIn, MsgOutArgs...>, IPC::Reader reader, Func&& func)
{
	typedef SyncMessage<MsgIn, MsgOutArgs...> Message;

	typename Message::Inputs inputs;
	typename Message::Outputs outputs;
	FillTuple<0>(inputs, reader);
	Util::apply(std::forward<Func>(func), std::tuple_cat(RefTuple(std::move(inputs)), RefTuple(outputs)));

	Writer writer;
	writer.Write<uint32_t>(ID_RETURN);
	SerializeArgs(writer, typename Message::Outputs(), outputs);
	socket.SendMsg(writer);
}

} // namespace detail

// Send a message to the given socket. If the message is synchronous then messageHandler is invoked for all
// message that are recieved until ID_RETURN is recieved. Values returned by a synchronous message are
// returned through reference parameters.
template<typename Msg, typename Func, typename... Args> void SendMsg(const Socket& socket, Func&& messageHandler, Args&&... args)
{
	detail::SendMsg(socket, messageHandler, Msg(), std::forward<Args>(args)...);
}

// Handle an incoming message using a callback function (which can just be a lambda). If the message is
// synchronous then outputs values are written to using reference parameters.
template<typename Msg, typename Func> void HandleMsg(const Socket& socket, Reader reader, Func&& func)
{
	detail::HandleMsg(socket, Msg(), std::move(reader), std::forward<Func>(func));
}

} // namespace IPC

#endif // COMMON_IPC_H_
