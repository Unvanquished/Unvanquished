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

#ifndef COMMON_RPC_H_
#define COMMON_RPC_H_

#include "../libs/nacl/nacl.h"

namespace RPC {

class Writer {
public:
	void Reset()
	{
		data.clear();
		handles.clear();
	}

	void Write(const void* p, size_t len)
	{
		data.insert(data.end(), static_cast<const char*>(p), static_cast<const char*>(p) + len);
	}
	void WriteHandle(NaCl::IPCHandle& h)
	{
		handles.push_back(&h);
	}

	void WriteInt(int x)
	{
		Write(&x, sizeof(x));
	}
	void WriteFloat(float x)
	{
		Write(&x, sizeof(x));
	}

	void WriteString(const char* p, size_t len)
	{
		WriteInt(len);
		Write(p, len);
		data.push_back('\0');
	}
	void WriteString(const char* p)
	{
		size_t len = strlen(p);
		WriteInt(len);
		Write(p, len + 1);
	}

	const char* GetData() const
	{
		return data.data();
	}
	size_t GetSize() const
	{
		return data.size();
	}
	NaCl::IPCHandle* const* GetHandles() const
	{
		return handles.data();
	}
	size_t GetNumHandles() const
	{
		return handles.size();
	}

private:
	std::vector<char> data;
	std::vector<NaCl::IPCHandle*> handles;
};

class Reader {
public:
	Reader()
	{
		Reset();
	}
	void Reset()
	{
		pos = 0;
		handles_pos = 0;
	}

	void Read(void* p, size_t len)
	{
		if (pos + len <= data.size()) {
			memcpy(p, &data[pos], len);
			pos += len;
		} else
			Com_Error(ERR_DROP, "RPC message too short");
	}
	const void* ReadInline(size_t len)
	{
		if (pos + len <= data.size()) {
			const void* out = &data[pos];
			pos += len;
			return out;
		} else
			Com_Error(ERR_DROP, "RPC message too short");
	}
	NaCl::IPCHandle ReadHandle()
	{
		if (handles_pos <= handles.size())
			return std::move(handles[handles_pos++]);
		else
			Com_Error(ERR_DROP, "RPC message too short");
	}

	int ReadInt()
	{
		int x;
		Read(&x, sizeof(x));
		return x;
	}
	float ReadFloat()
	{
		float x;
		Read(&x, sizeof(x));
		return x;
	}

	const char* ReadString(size_t& len)
	{
		len = ReadInt();
		const char* out = static_cast<const char*>(ReadInline(len + 1));
		if (out[len] != '\0')
			Com_Error(ERR_DROP, "RPC string not NUL-terminated");
		return out;
	}
	const char* ReadString()
	{
		size_t len;
		return ReadString(len);
	}

	std::vector<char>& GetDataBuffer()
	{
		return data;
	}
	std::vector<NaCl::IPCHandle>& GetHandlesBuffer()
	{
		return handles;
	}

private:
	std::vector<char> data;
	std::vector<NaCl::IPCHandle> handles;
	size_t pos;
	size_t handles_pos;
};

template<typename Func> Reader DoRPC(const NaCl::RootSocket& socket, Writer& writer, bool ignoreErrors, Func&& syscallHandler)
{
	Reader reader;

	if (!socket.SendMsg(writer.GetData(), writer.GetSize(), writer.GetHandles(), writer.GetNumHandles())) {
		if (ignoreErrors)
			return reader;
		Com_Error(ERR_DROP, "Error sending RPC message");
	}

	// Handle syscalls from the remote module until the "return" pseudo-syscall is invoked
	while (true) {
		reader.Reset();
		if (!socket.RecvMsg(reader.GetDataBuffer(), reader.GetHandlesBuffer()))
			Com_Error(ERR_DROP, "Error recieving RPC message");

		int syscall = reader.ReadInt();
		if (syscall == -1)
			return reader;

		writer.Reset();
		writer.WriteInt(-1);
		syscallHandler(syscall, reader, writer);
		if (!socket.SendMsg(writer.GetData(), writer.GetSize(), writer.GetHandles(), writer.GetNumHandles()))
			Com_Error(ERR_DROP, "Error sending RPC message");
	}
}

} // namespace RPC

#endif // COMMON_RPC_H_
