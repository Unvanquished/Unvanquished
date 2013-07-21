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

#ifndef RPC_H_
#define RPC_H_

#include <vector>
#include <memory>

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
		if (out[len - 1] != '\0')
			Com_Error(ERR_DROP, "RPC string not NUL-terminated");
		pos += len + 1;
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

#endif // RPC_H_
