/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2015, Daemon Developers
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

#include "../Common.h"

#ifndef COMMON_IPC_PRIMITIVES_H_
#define COMMON_IPC_PRIMITIVES_H_

namespace IPC {

    /*
     * NaCl allows us to share file descriptors between the engine and the VM,
     * these allow us in turn to share sockets and shared memory regions, which
     * are file descriptor-backed OS resources.
     */

	// Simple file handle wrapper, does *not* close handle on destruction
	enum FileOpenMode {
		MODE_READ,
		MODE_WRITE,
		MODE_RW,
		MODE_WRITE_APPEND,
		MODE_RW_APPEND
	};

	class FileHandle {
	public:
		FileHandle() : handle(Sys::INVALID_HANDLE) {}
		FileHandle(Sys::OSHandle handle, FileOpenMode mode) : handle(handle), mode(mode) {}
		explicit operator bool() const {
			return Sys::IsValidHandle(handle);
		}

		FileDesc GetDesc() const;
		static FileHandle FromDesc(const FileDesc& desc);

		Sys::OSHandle GetHandle() const {
			return handle;
		}

	private:
		Sys::OSHandle handle;
		FileOpenMode mode;
	};

	// Message-based socket through which data and handles can be passed.
	class Socket {
	public:
		Socket() : handle(Sys::INVALID_HANDLE) {}
		Socket(Socket&& other) NOEXCEPT : handle(other.handle) {
			other.handle = Sys::INVALID_HANDLE;
		}
		Socket& operator=(Socket&& other) NOEXCEPT {
			std::swap(handle, other.handle);
			return *this;
		}
		~Socket() {
			Close();
		}
		explicit operator bool() const {
			return Sys::IsValidHandle(handle);
		}

		void Close();

		Sys::OSHandle GetHandle() const {
			return handle;
		}
		Sys::OSHandle ReleaseHandle() {
			Sys::OSHandle out = handle;
			handle = Sys::INVALID_HANDLE;
			return out;
		}

		FileDesc GetDesc() const;
		static Socket FromDesc(const FileDesc& desc);
		static Socket FromHandle(Sys::OSHandle handle);

		void SendMsg(const Util::Writer& writer) const;
		Util::Reader RecvMsg() const;

		void SetRecvTimeout(std::chrono::nanoseconds timeout);

		static std::pair<Socket, Socket> CreatePair();

	private:
		Sys::OSHandle handle;
	};

	// Shared memory area, can be sent over a socket. Can be initialized in the VM
    // safely as the engine will ask the OS for the size of the Shared memory region.
	class SharedMemory {
	public:
		SharedMemory() : handle(Sys::INVALID_HANDLE) {}
		SharedMemory(SharedMemory&& other) NOEXCEPT : handle(other.handle), base(other.base), size(other.size) {
			other.handle = Sys::INVALID_HANDLE;
		}
		SharedMemory& operator=(SharedMemory&& other) NOEXCEPT {
			std::swap(handle, other.handle);
			std::swap(base, other.base);
			std::swap(size, other.size);
			return *this;
		}
		~SharedMemory() {
			Close();
		}
		explicit operator bool() const {
			return Sys::IsValidHandle(handle);
		}

		void Close();

		FileDesc GetDesc() const;
		static SharedMemory FromDesc(const FileDesc& desc);

		static SharedMemory Create(size_t size);

		void* GetBase() const {
			return base;
		}
		size_t GetSize() const {
			return size;
		}

	private:
		Sys::OSHandle handle;
		void* base;
		size_t size;
	};

} // namespace IPC

namespace Util {

	// Socket, file handle and shared memory
	template<> struct SerializeTraits<IPC::Socket> {
		static void Write(Writer& stream, const IPC::Socket& value)
		{
			stream.WriteHandle(value.GetDesc());
		}
		static IPC::Socket Read(Reader& stream)
		{
			return IPC::Socket::FromDesc(stream.ReadHandle());
		}
	};
	template<> struct SerializeTraits<IPC::FileHandle> {
		static void Write(Writer& stream, const IPC::FileHandle& value)
		{
			stream.WriteHandle(value.GetDesc());
		}
		static IPC::FileHandle Read(Reader& stream)
		{
			return IPC::FileHandle::FromDesc(stream.ReadHandle());
		}
	};
	template<> struct SerializeTraits<IPC::SharedMemory> {
		static void Write(Writer& stream, const IPC::SharedMemory& value)
		{
			stream.WriteHandle(value.GetDesc());
		}
		static IPC::SharedMemory Read(Reader& stream)
		{
			return IPC::SharedMemory::FromDesc(stream.ReadHandle());
		}
	};

} // namespace Util

#endif // COMMON_IPC_PRIMITIVES_H_

