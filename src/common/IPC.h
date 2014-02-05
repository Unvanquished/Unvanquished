/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

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

class Writer {
public:
	void Write(const void* p, size_t len)
	{
		data.insert(data.end(), static_cast<const char*>(p), static_cast<const char*>(p) + len);
	}
	void WriteSize(size_t size)
	{
		if (size > std::numeric_limits<uint32_t>::max())
			Com_Error(ERR_DROP, "Size out of range in IPC message");
		uint32_t realSize = size;
		Write(&realSize, sizeof(uint32_t));
	}
	void WriteHandle(NaCl::IPCHandle& h)
	{
		handles.push_back(&h);
	}

	void Reset()
	{
		data.clear();
		handles.clear();
	}
	const std::vector<char>& GetData() const
	{
		return data;
	}
	const std::vector<NaCl::IPCHandle*>& GetHandles() const
	{
		return handles;
	}

private:
	std::vector<char> data;
	std::vector<NaCl::IPCHandle*> handles;
};

class Reader {
public:
	void Read(void* p, size_t len)
	{
		if (pos + len <= data.size()) {
			memcpy(p, &data[pos], len);
			pos += len;
		} else
			Com_Error(ERR_DROP, "IPC message too short");
	}
	template<typename T> size_t ReadSize()
	{
		// Check for multiplication overflows when reading a size
		uint32_t size;
		Read(&size, sizeof(uint32_t));
		if (size > std::numeric_limits<uint32_t>::max() / sizeof(T))
			Com_Error(ERR_DROP, "Size out of range in IPC message");
		return size;
	}
	const void* ReadInline(size_t len)
	{
		if (pos + len <= data.size()) {
			const void* out = &data[pos];
			pos += len;
			return out;
		} else
			Com_Error(ERR_DROP, "IPC message too short");
	}
	NaCl::IPCHandle ReadHandle()
	{
		if (handles_pos <= handles.size())
			return std::move(handles[handles_pos++]);
		else
			Com_Error(ERR_DROP, "IPC message too short");
	}

	void Reset()
	{
		pos = 0;
		handles_pos = 0;
	}
	std::vector<char>& GetData()
	{
		return data;
	}
	std::vector<NaCl::IPCHandle>& GetHandles()
	{
		return handles;
	}

private:
	std::vector<char> data;
	std::vector<NaCl::IPCHandle> handles;
	size_t pos;
	size_t handles_pos;
};

// Base type for serialization traits.
template<typename T, typename = void> struct SerializeTraits {};

// Simple implementation for POD types
template<typename T>
struct SerializeTraits<T, typename std::enable_if<std::is_pod<T>::value>::type> {
	static void Write(Writer& stream, const T& value)
	{
		stream.Write(std::addressof(value), sizeof(value));
	}
	static T Read(Reader& stream)
	{
		T value;
		stream.Read(std::addressof(value), sizeof(value));
		return value;
	}
};

// std::vector, with a specialization for POD types
template<typename T>
struct SerializeTraits<std::vector<T>, typename std::enable_if<std::is_pod<T>::value>::type> {
	static void Write(Writer& stream, const std::vector<T>& value)
	{
		stream.WriteSize(value.size());
		stream.Write(value.data(), value.size() * sizeof(T));
	}
	static std::vector<T> Read(Reader& stream)
	{
		std::vector<T> value;
		value.resize(stream.ReadSize<T>());
		stream.Read(value.data(), value.size() * sizeof(T));
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
		stream.Write(value.data(), value.size());
	}
	static Str::StringRef Read(Reader& stream)
	{
		size_t size = stream.ReadSize<char>();
		const char* p = static_cast<const char*>(stream.ReadInline(size + 1));
		if (p[size] != '\0')
			Com_Error(ERR_DROP, "String in IPC message is not NUL-terminated");
		return p;
	}
};

// IPC message, which automatically serializes and deserializes objects
template<uint32_t Major, uint32_t Minor, typename... T> class Message {
	typedef std::tuple<T...> TupleType;

	template<size_t Index, typename Tuple> static void FillTuple(Tuple&, Reader&) {}
	template<size_t Index, typename Type0, typename... Types, typename Tuple> static void FillTuple(Tuple& tuple, Reader& stream)
	{
		std::get<Index>(tuple) = SerializeTraits<Type0>::Read(stream);
		FillTuple<Index + 1, Types...>(tuple, stream);
	}

	template<size_t Index> static void SerializeImpl(Writer&) {}
	template<size_t Index, typename Arg0, typename... Args> void SerializeImpl(Arg0&& arg0, Args&&... args)
	{
		SerializeTraits<typename std::tuple_element<Index, TupleType>::type>::Write(stream, std::forward<Arg0>(arg0));
		SerializeImpl<Index + 1>(std::forward<Args>(args)...);
	}

public:
	// Stream containing the serialized message data
	Writer stream;

	// Construct a message for sending
	template<typename... Args> Message(Args&&... args)
	{
		static_assert(sizeof...(Args) == sizeof...(T), "Incorrect number of arguments for IPC::Message::Serialize");
		SerializeTraits<uint32_t>::Write(stream, Major);
		SerializeTraits<uint32_t>::Write(stream, Minor);
		SerializeImpl<0>(stream, std::forward<Args>(args)...);
	}

	// Deserialize a message and call the given function with the message arguments
	template<typename Func> static void Deserialize(Reader& stream, Func&& func)
	{
		std::tuple<decltype(SerializeTraits<T>::Read(stream))...> tuple;
		FillTuple<0, T...>(tuple, stream);
		Util::apply(std::forward<Func>(func), std::move(tuple));
	}
};

#endif // COMMON_IPC_H_
