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

#ifndef COMMON_SERIALIZATION_H_
#define COMMON_SERIALIZATION_H_

#include "IPC/Common.h"

namespace Util {

    /*
     * Contains classes used to serialize data to memory and back. Also to
     * support NaCl IPC specifics, they can contain IPC file descriptors.
     * To define serialization and deserialization, all is needed is to
     * implement the following template trait.
     *
     * namespace Util {
     *     template<>
     *     struct SerializeTraits<MyType> {
     *         static void Write(Writer& stream, const MyType& value) {
     *             // Write data to stream
     *         }
     *         static MyType Read(Reader& stream) {
     *             // Read from stream and return
     *         }
     *     };
     * }
     */

	// Trait declaration for the serialization trait.
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
				Sys::Drop("IPC: Size out of range in message");
			Write<uint32_t>(size);
		}
		template<typename T, typename Arg> void Write(Arg&& value)
		{
			SerializeTraits<T>::Write(*this, std::forward<Arg>(value));
		}
		void WriteHandle(const IPC::FileDesc& h)
		{
			handles.push_back(h);
		}

		const std::vector<char>& GetData() const
		{
			return data;
		}
		const std::vector<IPC::FileDesc>& GetHandles() const
		{
			return handles;
		}

		// Serialize a list of types into a Writer (ignores extra trailing arguments)
		template<typename... Args>
		void WriteArgs(Util::TypeList<>, Args&&...) {}

		template<typename Type0, typename... Types, typename Arg0, typename... Args>
		void WriteArgs(Util::TypeList<Type0, Types...>, Arg0&& arg0, Args&&... args)
		{
			Write<Type0>(std::forward<Arg0>(arg0));
			WriteArgs(Util::TypeList<Types...>(), std::forward<Args>(args)...);
		}
		template<typename Types, typename Tuple, size_t... Seq>
		void WriteTuple(Types, Tuple&& tuple, Util::seq<Seq...>)
		{
			WriteArgs(Types(), std::get<Seq>(std::forward<Tuple>(tuple))...);
		}
		template<typename Types, typename Tuple>
		void WriteTuple(Types, Tuple&& tuple)
		{
			WriteTuple(Types(), std::forward<Tuple>(tuple), Util::gen_seq<std::tuple_size<typename std::decay<Tuple>::type>::value>());
		}
	private:
		std::vector<char> data;
		std::vector<IPC::FileDesc> handles;
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
				handles[i].Close();
		}

		void ReadData(void* p, size_t len)
		{
			if (pos + len <= data.size()) {
				memcpy(p, data.data() + pos, len);
				pos += len;
			} else
				Sys::Drop("IPC: Unexpected end of message");
		}
		template<typename T> size_t ReadSize()
		{
			// Check for multiplication overflows when reading a size
			uint32_t size;
			ReadData(&size, sizeof(uint32_t));
			if (size > std::numeric_limits<uint32_t>::max() / sizeof(T))
				Sys::Drop("IPC: Size out of range in message");
			return size;
		}
		const void* ReadInline(size_t len)
		{
			if (pos + len <= data.size()) {
				const void* out = data.data() + pos;
				pos += len;
				return out;
			} else
				Sys::Drop("IPC: Unexpected end of message");
		}
		template<typename T> decltype(SerializeTraits<T>::Read(std::declval<Reader&>())) Read()
		{
			return SerializeTraits<T>::Read(*this);
		}

		// Read a list of types into a tuple starting at the given index
		template<size_t Index, typename Tuple>
		void FillTuple(Util::TypeList<>, Tuple&) {}
		template<size_t Index, typename Type0, typename... Types, typename Tuple>
		void FillTuple(Util::TypeList<Type0, Types...>, Tuple& tuple)
		{
			std::get<Index>(tuple) = Read<Type0>();
			FillTuple<Index + 1>(Util::TypeList<Types...>(), tuple);
		}

		IPC::FileDesc ReadHandle()
		{
			if (handles_pos <= handles.size())
				return handles[handles_pos++];
			else
				Sys::Drop("Reader: Unexpected end of message");
		}

		void CheckEndRead()
		{
			if (pos != data.size())
				Sys::Drop("Reader: Unread bytes at end of message");
			if (handles_pos != handles.size())
				Sys::Drop("Reader: Unread handles at end of message");
		}

		std::vector<char>& GetData()
		{
			return data;
		}
		std::vector<IPC::FileDesc>& GetHandles()
		{
			return handles;
		}

	private:
		std::vector<char> data;
		std::vector<IPC::FileDesc> handles;
		size_t pos;
		size_t handles_pos;
	};

    // Implementation of the serialization traits for common types and std containers

	// Simple implementation for POD types
	template<typename T>
	struct SerializeTraits<T, typename std::enable_if<Util::IsPOD<T>::value && !std::is_array<T>::value>::type> {
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

	// std::array for non-POD types (POD types are already handled by the base case)
	template<typename T, size_t N>
	struct SerializeTraits<std::array<T, N>, typename std::enable_if<!Util::IsPOD<T>::value>::type> {
		static void Write(Writer& stream, const std::array<T, N>& value)
		{
			for (const T& x: value)
				stream.Write<T>(x);
		}
		static std::array<T, N> Read(Reader& stream)
		{
			std::array<T, N> value;
			for (T& x: value)
				x = stream.Read<T>();
			return value;
		}
	};

	// std::vector, with a specialization for POD types
	template<typename T>
	struct SerializeTraits<std::vector<T>, typename std::enable_if<Util::IsPOD<T>::value>::type> {
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
	struct SerializeTraits<std::vector<T>, typename std::enable_if<!Util::IsPOD<T>::value>::type> {
		static void Write(Writer& stream, const std::vector<T>& value)
		{
			stream.WriteSize(value.size());
			for (const T& x: value)
				stream.Write<T>(x);
		}
		static std::vector<T> Read(Reader& stream)
		{
			std::vector<T> value;
			value.resize(stream.ReadSize<T>());
			for (T& x: value)
				x = stream.Read<T>();
			return value;
		}
	};
	template<>
	struct SerializeTraits<std::vector<bool>> {
		static void Write(Writer& stream, const std::vector<bool>& value)
		{
			stream.WriteSize(value.size());
			for (bool x: value)
				stream.Write<bool>(x);
		}
		static std::vector<bool> Read(Reader& stream)
		{
			std::vector<bool> value;
			value.resize(stream.ReadSize<bool>());
			for (auto i = value.begin(); i != value.end(); ++i)
				*i = stream.Read<bool>();
			return value;
		}
	};

	// std::pair
	template<typename T, typename U>
	struct SerializeTraits<std::pair<T, U>> {
		static void Write(Writer& stream, const std::pair<T, U>& value)
		{
			stream.Write<T>(value.first);
			stream.Write<U>(value.second);
		}
		static std::pair<T, U> Read(Reader& stream)
		{
			std::pair<T, U> value;
			value.first = stream.Read<T>();
			value.second = stream.Read<U>();
			return value;
		}
	};

	// Util::optional
	template<typename T>
	struct SerializeTraits<Util::optional<T>> {
		static void Write(Writer& stream, const Util::optional<T>& value)
		{
			stream.Write<bool>(bool(value));
			if (value)
				stream.Write<T>(*value);
		}
		static Util::optional<T> Read(Reader& stream)
		{
			if (stream.Read<bool>())
				return stream.Read<T>();
			else
				return Util::nullopt;
		}
	};

	// std::string
	template<> struct SerializeTraits<std::string> {
		static void Write(Writer& stream, Str::StringRef value)
		{
			stream.WriteSize(value.size());
			stream.WriteData(value.data(), value.size());
		}
		static std::string Read(Reader& stream)
		{
			size_t size = stream.ReadSize<char>();
			const char* p = static_cast<const char*>(stream.ReadInline(size));
			return std::string(p, p + size);
		}
	};

	// std::map and std::unordered_map
	template<typename T, typename U>
	struct SerializeTraits<std::map<T, U>> {
		static void Write(Writer& stream, const std::map<T, U>& value)
		{
			stream.WriteSize(value.size());
			for (const std::pair<T, U>& x: value)
				stream.Write<std::pair<T, U>>(x);
		}
		static std::map<T, U> Read(Reader& stream)
		{
			std::map<T, U> value;
			size_t size = stream.ReadSize<std::pair<T, U>>();
			for (size_t i = 0; i != size; i++)
				value.insert(stream.Read<std::pair<T, U>>());
			return value;
		}
	};
	template<typename T, typename U>
	struct SerializeTraits<std::unordered_map<T, U>> {
		static void Write(Writer& stream, const std::unordered_map<T, U>& value)
		{
			stream.WriteSize(value.size());
			for (const std::pair<T, U>& x: value)
				stream.Write<std::pair<T, U>>(x);
		}
		static std::unordered_map<T, U> Read(Reader& stream)
		{
			std::unordered_map<T, U> value;
			size_t size = stream.ReadSize<std::pair<T, U>>();
			for (size_t i = 0; i != size; i++)
				value.insert(stream.Read<std::pair<T, U>>());
			return value;
		}
	};

	// std::set and std::unordered_set
	template<typename T>
	struct SerializeTraits<std::set<T>> {
		static void Write(Writer& stream, const std::set<T>& value)
		{
			stream.WriteSize(value.size());
			for (const T& x: value)
				stream.Write<T>(x);
		}
		static std::set<T> Read(Reader& stream)
		{
			std::set<T> value;
			size_t size = stream.ReadSize<T>();
			for (size_t i = 0; i != size; i++)
				value.insert(stream.Read<T>());
			return value;
		}
	};
	template<typename T>
	struct SerializeTraits<std::unordered_set<T>> {
		static void Write(Writer& stream, const std::unordered_set<T>& value)
		{
			stream.WriteSize(value.size());
			for (const T& x: value)
				stream.Write<T>(x);
		}
		static std::unordered_set<T> Read(Reader& stream)
		{
			std::unordered_set<T> value;
			size_t size = stream.ReadSize<T>();
			for (size_t i = 0; i != size; i++)
				value.insert(stream.Read<T>());
			return value;
		}
	};

} // namespace Util

#endif // COMMON_SERIALIZATION_H_
