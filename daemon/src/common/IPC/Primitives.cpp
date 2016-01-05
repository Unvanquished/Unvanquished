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
#include "Primitives.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifndef __native_client__
#include <sys/socket.h>
#endif
#endif

#ifdef BUILD_VM
#include "shared/VMMain.h"
#include "CommonSyscalls.h"
#endif

#undef INLINE
#undef DLLEXPORT
#undef NORETURN
#undef NORETURN_PTR
#include "nacl/native_client/src/shared/imc/nacl_imc_c.h"
#include "nacl/native_client/src/public/imc_types.h"
#include "nacl/native_client/src/trusted/service_runtime/nacl_config.h"
#include "nacl/native_client/src/trusted/service_runtime/include/sys/fcntl.h"

// Definitions taken from nacl_desc_base.h
enum NaClDescTypeTag {
  NACL_DESC_INVALID,
  NACL_DESC_DIR,
  NACL_DESC_HOST_IO,
  NACL_DESC_CONN_CAP,
  NACL_DESC_CONN_CAP_FD,
  NACL_DESC_BOUND_SOCKET,
  NACL_DESC_CONNECTED_SOCKET,
  NACL_DESC_SHM,
  NACL_DESC_SYSV_SHM,
  NACL_DESC_MUTEX,
  NACL_DESC_CONDVAR,
  NACL_DESC_SEMAPHORE,
  NACL_DESC_SYNC_SOCKET,
  NACL_DESC_TRANSFERABLE_DATA_SOCKET,
  NACL_DESC_IMC_SOCKET,
  NACL_DESC_QUOTA,
  NACL_DESC_DEVICE_RNG,
  NACL_DESC_DEVICE_POSTMESSAGE,
  NACL_DESC_CUSTOM,
  NACL_DESC_NULL
};
#define NACL_DESC_TYPE_MAX      (NACL_DESC_NULL + 1)
#define NACL_DESC_TYPE_END_TAG  (0xff)

struct NaClInternalRealHeader {
  uint32_t  xfer_protocol_version;
  uint32_t  descriptor_data_bytes;
};

struct NaClInternalHeader {
  struct NaClInternalRealHeader h;
  char      pad[((sizeof(struct NaClInternalRealHeader) + 0x10) & ~0xf)
                - sizeof(struct NaClInternalRealHeader)];
};

#define NACL_HANDLE_TRANSFER_PROTOCOL 0xd3c0de01
// End of imported definitions

namespace IPC {

void FileDesc::Close() const
{
	NaClClose(handle);
}

FileDesc FileHandle::GetDesc() const
{
	FileDesc out;
#ifdef _WIN32
	out.handle = reinterpret_cast<HANDLE>(_get_osfhandle(handle));
#else
	out.handle = handle;
#endif
#ifndef __native_client__
	out.type = NACL_DESC_HOST_IO;
	switch (mode) {
	case MODE_READ:
		out.flags = NACL_ABI_O_RDONLY;
		break;
	case MODE_WRITE:
		out.flags = NACL_ABI_O_WRONLY;
		break;
	case MODE_RW:
		out.flags = NACL_ABI_O_RDWR;
		break;
	case MODE_WRITE_APPEND:
		out.flags = NACL_ABI_O_WRONLY | NACL_ABI_O_APPEND;
		break;
	case MODE_RW_APPEND:
		out.flags = NACL_ABI_O_RDWR | NACL_ABI_O_APPEND;
		break;
	}
#endif
	return out;
}

FileHandle FileHandle::FromDesc(const FileDesc& desc)
{
	FileOpenMode mode = MODE_READ;
#ifndef __native_client__
	switch (desc.flags) {
	case NACL_ABI_O_RDONLY:
		mode = MODE_READ;
		break;
	case NACL_ABI_O_WRONLY:
		mode = MODE_WRITE;
		break;
	case NACL_ABI_O_RDWR:
		mode = MODE_RW;
		break;
	case NACL_ABI_O_WRONLY | NACL_ABI_O_APPEND:
		mode = MODE_WRITE_APPEND;
		break;
	case NACL_ABI_O_RDWR | NACL_ABI_O_APPEND:
		mode = MODE_RW_APPEND;
		break;
	}
#endif
#ifdef _WIN32
	int modes[] = {O_RDONLY, O_WRONLY | O_TRUNC | O_CREAT, O_WRONLY | O_APPEND | O_CREAT, O_RDWR | O_CREAT};
	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(desc.handle), modes[mode]);
	if (fd == -1) {
		CloseHandle(desc.handle);
		return FileHandle();
	}
	return FileHandle(fd, mode);
#else
	return FileHandle(desc.handle, mode);
#endif
}

OwnedFileHandle::~OwnedFileHandle()
{
	close(handle.GetHandle());
}

void Socket::Close()
{
	if (Sys::IsValidHandle(handle))
		NaClClose(handle);
	handle = Sys::INVALID_HANDLE;
}

FileDesc Socket::GetDesc() const
{
	FileDesc out;
	out.handle = handle;
#ifndef __native_client__
	out.type = NACL_DESC_TRANSFERABLE_DATA_SOCKET;
#endif
	return out;
}

Socket Socket::FromDesc(const FileDesc& desc)
{
	Socket out;
	out.handle = desc.handle;
	return out;
}
Socket Socket::FromHandle(Sys::OSHandle handle)
{
	Socket out;
	out.handle = handle;
	return out;
}

static void InternalSendMsg(Sys::OSHandle handle, bool more, const FileDesc* handles, size_t numHandles, const void* data, size_t len)
{
	NaClMessageHeader hdr;
	NaClIOVec iov[4];

	NaClHandle h[NACL_ABI_IMC_DESC_MAX];
	for (size_t i = 0; i < numHandles; i++) {
		if (!Sys::IsValidHandle(handles[i].handle))
			Sys::Drop("IPC: Tried to send an invalid handle");
		h[i] = handles[i].handle;
	}

#ifdef __native_client__
	hdr.iov = iov;
	hdr.iov_length = 2;
	hdr.handles = h;
	hdr.handle_count = numHandles;
	hdr.flags = 0;
	iov[0].base = &more;
	iov[0].length = 1;
	iov[1].base = const_cast<void*>(data);
	iov[1].length = len;
	if (NaClSendDatagram(handle, &hdr, 0) == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to send message: %s", error);
	}
#else
	size_t descBytes = 0;
	std::unique_ptr<unsigned char[]> descBuffer;
	if (numHandles != 0) {
		for (size_t i = 0; i < numHandles; i++) {
			// tag: 1 byte
			// flags: 4 bytes
			// size: 8 bytes (only for SHM)
			descBytes++;
			descBytes += sizeof(uint32_t);
			if (handles[i].type == NACL_DESC_SHM)
				descBytes += sizeof(uint64_t);
			else if (handles[i].type == NACL_DESC_HOST_IO)
				descBytes += sizeof(int32_t);
		}
		// Add 1 byte end tag and round to 16 bytes
		descBytes = (descBytes + 1 + 0xf) & ~0xf;

		descBuffer.reset(new unsigned char[descBytes]);
		unsigned char* descBuffer_ptr = &descBuffer[0];
		for (size_t i = 0; i < numHandles; i++) {
			*descBuffer_ptr++ = handles[i].type;
			memset(descBuffer_ptr, 0, sizeof(uint32_t));
			descBuffer_ptr += sizeof(uint32_t);
			if (handles[i].type == NACL_DESC_SHM) {
				memcpy(descBuffer_ptr, &handles[i].size, sizeof(uint64_t));
				descBuffer_ptr += sizeof(uint64_t);
			} else if (handles[i].type == NACL_DESC_HOST_IO) {
				memcpy(descBuffer_ptr, &handles[i].flags, sizeof(int32_t));
				descBuffer_ptr += sizeof(int32_t);
			}
		}
		*descBuffer_ptr++ = NACL_DESC_TYPE_END_TAG;

		// Clear any padding bytes to avoid information leak
		std::fill(descBuffer_ptr, &descBuffer[descBytes], 0);
	}

	NaClInternalHeader internalHdr = {{NACL_HANDLE_TRANSFER_PROTOCOL, static_cast<uint32_t>(descBytes)}, {}};
	hdr.iov = iov;
	hdr.iov_length = 4;
	hdr.handles = h;
	hdr.handle_count = numHandles;
	hdr.flags = 0;
	iov[0].base = &internalHdr;
	iov[0].length = sizeof(NaClInternalHeader);
	iov[1].base = descBuffer.get();
	iov[1].length = descBytes;
	iov[2].base = &more;
	iov[2].length = 1;
	iov[3].base = const_cast<void*>(data);
	iov[3].length = len;

	if (NaClSendDatagram(handle, &hdr, 0) == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to send message: %s", error);
	}
#endif
}

void Socket::SendMsg(const Util::Writer& writer) const
{
	const FileDesc* handles = writer.GetHandles().data();
	size_t numHandles = writer.GetHandles().size();
	const void* data = writer.GetData().data();
	size_t len = writer.GetData().size();

	while (numHandles || len) {
		bool more = numHandles > NACL_ABI_IMC_DESC_MAX || len > NACL_ABI_IMC_USER_BYTES_MAX - 1;
		InternalSendMsg(handle, more, handles, std::min<size_t>(numHandles, NACL_ABI_IMC_DESC_MAX), data, std::min<size_t>(len, NACL_ABI_IMC_USER_BYTES_MAX - 1));
		handles += std::min<size_t>(numHandles, NACL_ABI_IMC_DESC_MAX);
		numHandles -= std::min<size_t>(numHandles, NACL_ABI_IMC_DESC_MAX);
		data = static_cast<const char*>(data) + std::min<size_t>(len, NACL_ABI_IMC_USER_BYTES_MAX - 1);
		len -= std::min<size_t>(len, NACL_ABI_IMC_USER_BYTES_MAX - 1);
	}
}

#ifndef __native_client__
// Clean up handles on error so that they don't leak
static void FreeHandles(const NaClHandle* h)
{
	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
		if (h[i] != NACL_INVALID_HANDLE)
			NaClClose(h[i]);
	}
}
#endif

bool InternalRecvMsg(Sys::OSHandle handle, Util::Reader& reader)
{
	NaClMessageHeader hdr;
	NaClIOVec iov[2];
	NaClHandle h[NACL_ABI_IMC_DESC_MAX];
	std::unique_ptr<char[]> recvBuffer(new char[NACL_ABI_IMC_BYTES_MAX]);

	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++)
		h[i] = NACL_INVALID_HANDLE;

#ifdef __native_client__
	hdr.iov = iov;
	hdr.iov_length = 1;
	hdr.handles = h;
	hdr.handle_count = NACL_ABI_IMC_DESC_MAX;
	hdr.flags = 0;
	iov[0].base = recvBuffer.get();
	iov[0].length = NACL_ABI_IMC_BYTES_MAX;

	int result = NaClReceiveDatagram(handle, &hdr, 0);
	if (result == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to receive message: %s", error);
	}

	if (hdr.flags & (NACL_MESSAGE_TRUNCATED | NACL_HANDLES_TRUNCATED))
		Sys::Drop("IPC: Recieved truncated message");

	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
		if (h[i] != NACL_INVALID_HANDLE) {
			reader.GetHandles().emplace_back();
			reader.GetHandles().back().handle = h[i];
		}
	}
	reader.GetData().insert(reader.GetData().end(), &recvBuffer[1], &recvBuffer[result]);
	return recvBuffer[0];
#else
	NaClInternalHeader internalHdr;
	hdr.iov = iov;
	hdr.iov_length = 2;
	hdr.handles = h;
	hdr.handle_count = NACL_ABI_IMC_DESC_MAX;
	hdr.flags = 0;
	iov[0].base = &internalHdr;
	iov[0].length = sizeof(NaClInternalHeader);
	iov[1].base = &recvBuffer[0];
	iov[1].length = NACL_ABI_IMC_BYTES_MAX;

	int result = NaClReceiveDatagram(handle, &hdr, 0);
	if (result == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to receive message: %s", error);
	}

	if (hdr.flags & (NACL_MESSAGE_TRUNCATED | NACL_HANDLES_TRUNCATED))
		Sys::Drop("IPC: Recieved truncated message");

	if (result == 0) {
		FreeHandles(h);
		Sys::Drop("IPC: Socket closed by remote end");
	}

	if (result < (int)sizeof(NaClInternalHeader)) {
		FreeHandles(h);
		Sys::Drop("IPC: Message with invalid header");
	}
	result -= sizeof(NaClInternalHeader);

	if (internalHdr.h.xfer_protocol_version != NACL_HANDLE_TRANSFER_PROTOCOL || result < (int)internalHdr.h.descriptor_data_bytes) {
		FreeHandles(h);
		Sys::Drop("IPC: Message with invalid header");
	}
	result -= internalHdr.h.descriptor_data_bytes;

	char* desc_ptr = &recvBuffer[0];
	char* desc_end = desc_ptr + internalHdr.h.descriptor_data_bytes;
	size_t handle_index = 0;
	while (desc_ptr < desc_end) {
		int tag = 0xff & *desc_ptr++;
		if (tag == NACL_DESC_TYPE_END_TAG)
			break;
		if (tag != NACL_DESC_SHM && tag != NACL_DESC_TRANSFERABLE_DATA_SOCKET && tag != NACL_DESC_HOST_IO) {
			FreeHandles(h);
			Sys::Drop("IPC: Unrecognized descriptor tag in message: %02x", tag);
		}

		// Ignore flags
		if (desc_end - desc_ptr < (int)sizeof(uint32_t)) {
			FreeHandles(h);
			Sys::Drop("IPC: Descriptor flags missing from message");
		}
		desc_ptr += sizeof(uint32_t);

		uint64_t size = 0;
		int32_t flags = 0;
		if (tag == NACL_DESC_SHM) {
			if (desc_end - desc_ptr < (int)sizeof(uint64_t)) {
				FreeHandles(h);
				Sys::Drop("IPC: Shared memory size missing from message");
			}
			memcpy(&size, desc_ptr, sizeof(uint64_t));
			desc_ptr += sizeof(uint64_t);
		} else if (tag == NACL_DESC_HOST_IO) {
			if (desc_end - desc_ptr < (int)sizeof(int32_t)) {
				FreeHandles(h);
				Sys::Drop("IPC: Host file mode missing from message");
			}
			memcpy(&flags, desc_ptr, sizeof(int32_t));
			desc_ptr += sizeof(int32_t);
		}

		size_t i = handle_index++;
		reader.GetHandles().emplace_back();
		reader.GetHandles().back().handle = h[i];
		reader.GetHandles().back().type = tag;
		h[i] = NACL_INVALID_HANDLE;
		if (tag == NACL_DESC_SHM)
			reader.GetHandles().back().size = size;
		else if (tag == NACL_DESC_HOST_IO)
			reader.GetHandles().back().flags = flags;
	}

	reader.GetData().insert(reader.GetData().end(), &desc_end[1], &desc_end[result]);
	return desc_end[0];
#endif
}

Util::Reader Socket::RecvMsg() const
{
    Util::Reader out;
	while (InternalRecvMsg(handle, out)) {}
	return out;
}

void Socket::SetRecvTimeout(std::chrono::nanoseconds timeout)
{
#if defined(_WIN32) || defined(__native_client__)
	// Not supported on Windows or NaCl
	Q_UNUSED(timeout);
#else
	struct timeval tv;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
	tv.tv_sec = seconds.count();
	tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds).count();
	setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

std::pair<Socket, Socket> Socket::CreatePair()
{
	NaClHandle handles[2];
	if (NaClSocketPair(handles) != 0) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to create socket pair: %s", error);
	}
	Socket a, b;
	a.handle = handles[0];
	b.handle = handles[1];
	return std::make_pair(std::move(a), std::move(b));
}

static void* MapSharedMemory(Sys::OSHandle handle, size_t size)
{
	// We don't use NaClMap here because it only supports MAP_FIXED
#ifdef _WIN32
	void* base = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (base == nullptr)
		Sys::Drop("IPC: Failed to map shared memory object of size %zu: %s", size, Sys::Win32StrError(GetLastError()));
	return base;
#else
	void* base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
	if (base == MAP_FAILED)
		Sys::Drop("IPC: Failed to map shared memory object of size %zu: %s", size, strerror(errno));
	return base;
#endif
}

void SharedMemory::Close()
{
	if (Sys::IsValidHandle(handle)) {
#ifdef _WIN32
		UnmapViewOfFile(base);
#else
		munmap(base, size);
#endif
		NaClClose(handle);
	}
	handle = Sys::INVALID_HANDLE;
}

FileDesc SharedMemory::GetDesc() const
{
	FileDesc out;
	out.handle = handle;
#ifndef __native_client__
	out.type = NACL_DESC_SHM;
	out.size = size;
#endif
	return out;
}

SharedMemory SharedMemory::FromDesc(const FileDesc& desc)
{
	SharedMemory out;
#ifdef __native_client__
	struct stat st;
	if (fstat(desc.handle, &st) == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to stat shared memory handle: %s", error);
	}
	out.size = st.st_size;
#else
	out.size = desc.size;
#endif
	out.handle = desc.handle;
	out.base = MapSharedMemory(out.handle, out.size);
	return out;
}

#ifdef BUILD_VM
SharedMemory SharedMemory::Create(size_t size)
{
    IPC::SharedMemory result;
    VM::SendMsg<VM::CreateSharedMemoryMsg>(size, result);
    return result;
}
#else
SharedMemory SharedMemory::Create(size_t size)
{
	// Round size up to page size, otherwise the syscall will fail in NaCl
	size = (size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);

	Sys::OSHandle handle = NaClCreateMemoryObject(size, 0);
	if (!Sys::IsValidHandle(handle)) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Sys::Drop("IPC: Failed to create shared memory object of size %zu: %s", size, error);
	}

	SharedMemory out;
	out.handle = handle;
	out.size = size;
	out.base = MapSharedMemory(out.handle, out.size);
	return out;
}
#endif

} // namespace IPC
