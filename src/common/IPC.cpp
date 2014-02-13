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

#include "IPC.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#undef INLINE
#undef DLLEXPORT
#undef NORETURN
#undef NORETURN_PTR
#include "../libs/nacl/native_client/src/shared/imc/nacl_imc_c.h"
#include "../libs/nacl/native_client/src/public/imc_types.h"
#include "../libs/nacl/native_client/src/trusted/service_runtime/nacl_config.h"
#include "../libs/nacl/native_client/src/trusted/service_runtime/include/sys/fcntl.h"

// Make sure if we notice if any of our assumptions about NaCl are violated
static_assert(std::is_same<IPC::OSHandleType, NaClHandle>::value, "NaClHandle");
static_assert(IPC::INVALID_HANDLE == NACL_INVALID_HANDLE, "NACL_INVALID_HANDLE");
static_assert(IPC::MSG_MAX_BYTES == NACL_ABI_IMC_USER_BYTES_MAX, "NACL_ABI_IMC_USER_BYTES_MAX");
static_assert(IPC::MSG_MAX_HANDLES == NACL_ABI_IMC_DESC_MAX, "NACL_ABI_IMC_DESC_MAX");

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
#define NACL_DESC_TYPE_END_TAG  ((char)0xff)

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

void CloseDesc(const Desc& desc)
{
#ifdef __native_client__
	NaClClose(desc);
#else
	NaClClose(desc.handle);
#endif
}

Desc DescFromFile(OSHandleType handle, FileOpenMode mode)
{
#ifdef __native_client__
	return handle;
#else
	Desc out;
	out.type = NACL_DESC_HOST_IO;
	out.handle = handle;
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
	return out;
#endif
}
OSHandleType DescToHandle(const Desc& desc)
{
#ifdef __native_client__
	return desc;
#else
	return desc.handle;
#endif
}

void Socket::Close()
{
	if (handle != INVALID_HANDLE)
		NaClClose(handle);
	handle = INVALID_HANDLE;
}

Desc Socket::GetDesc() const
{
#ifdef __native_client__
	return handle;
#else
	Desc out;
	out.type = NACL_DESC_TRANSFERABLE_DATA_SOCKET;
	out.handle = handle;
	return out;
#endif
}
Socket Socket::FromDesc(const Desc& desc)
{
	Socket out;
#ifdef __native_client__
	out.handle = desc;
#else
	out.handle = desc.handle;
#endif
	return out;
}

void Socket::SendMsg(const Writer& writer) const
{
	NaClMessageHeader hdr;
	NaClIOVec iov[3];
	NaClHandle h[NACL_ABI_IMC_DESC_MAX];
	const Desc* handles = writer.GetHandles().data();
	size_t numHandles = writer.GetHandles().size();
	const void* data = writer.GetHandles().data();
	size_t len = writer.GetHandles().size();

	if (numHandles > NACL_ABI_IMC_DESC_MAX)
		Com_Error(ERR_DROP, "IPC: Message contains more handles than maximum allowed: %lu\n", numHandles);
	if (len > NACL_ABI_IMC_USER_BYTES_MAX)
		Com_Error(ERR_DROP, "IPC: Message above maximum size: %lu\n", len);
	for (size_t i = 0; i < numHandles; i++)
		h[i] = DescToHandle(handles[i]);

#ifdef __native_client__
	hdr.iov = iov;
	hdr.iov_length = 1;
	hdr.handles = h;
	hdr.handle_count = numHandles;
	hdr.flags = 0;
	iov[0].base = const_cast<void*>(data);
	iov[0].length = len;
	if (NaClSendDatagram(handle, &hdr, 0) == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to send message: %s", error);
	}
#else
	size_t descBytes = 0;
	std::unique_ptr<char[]> descBuffer;
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

		descBuffer.reset(new char[descBytes]);
		char* descBuffer_ptr = &descBuffer[0];
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
	hdr.iov_length = 3;
	hdr.handles = h;
	hdr.handle_count = numHandles;
	hdr.flags = 0;
	iov[0].base = &internalHdr;
	iov[0].length = sizeof(NaClInternalHeader);
	iov[1].base = descBuffer.get();
	iov[1].length = descBytes;
	iov[2].base = const_cast<void*>(data);
	iov[2].length = len;

	if (NaClSendDatagram(handle, &hdr, 0) == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to send message: %s", error);
	}
#endif
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

Reader Socket::RecvMsg() const
{
	NaClMessageHeader hdr;
	NaClIOVec iov[2];
	NaClHandle h[NACL_ABI_IMC_DESC_MAX];
	std::unique_ptr<char[]> recvBuffer(new char[NACL_ABI_IMC_BYTES_MAX]);
	Reader out;

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

	int result = NaClRecieveDatagram(handle, &hdr, 0);
	if (result == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to receive message: %s", error);
	}

	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
		if (h[i] != NACL_INVALID_HANDLE)
			out.GetHandles().push_back(h[i]);
	}
	out.GetData().assign(&recvBuffer[0], &recvBuffer[result]);
	return out;
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
		Com_Error(ERR_DROP, "IPC: Failed to receive message: %s", error);
	}

	if (result < (int)sizeof(NaClInternalHeader)) {
		FreeHandles(h);
		Com_Error(ERR_DROP, "IPC: Message with invalid header");
	}
	result -= sizeof(NaClInternalHeader);

	if (internalHdr.h.xfer_protocol_version != NACL_HANDLE_TRANSFER_PROTOCOL || result < (int)internalHdr.h.descriptor_data_bytes) {
		FreeHandles(h);
		Com_Error(ERR_DROP, "IPC: Message with invalid header");
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
			Com_Error(ERR_DROP, "IPC: Unrecognized descriptor tag in message: %02x", tag);
		}

		// Ignore flags
		if (desc_end - desc_ptr < (int)sizeof(uint32_t)) {
			FreeHandles(h);
			Com_Error(ERR_DROP, "IPC: Descriptor flags missing from message");
		}
		desc_ptr += sizeof(uint32_t);

		uint64_t size = 0;
		int32_t flags = 0;
		if (tag == NACL_DESC_SHM) {
			if (desc_end - desc_ptr < (int)sizeof(uint64_t)) {
				FreeHandles(h);
				Com_Error(ERR_DROP, "IPC: Shared memory size missing from message");
			}
			memcpy(&size, desc_ptr, sizeof(uint64_t));
			desc_ptr += sizeof(uint64_t);
		} else if (tag == NACL_DESC_HOST_IO) {
			if (desc_end - desc_ptr < (int)sizeof(int32_t)) {
				FreeHandles(h);
				Com_Error(ERR_DROP, "IPC: Host file mode missing from message");
			}
			memcpy(&flags, desc_ptr, sizeof(int32_t));
			desc_ptr += sizeof(int32_t);
		}

		size_t i = handle_index++;
		out.GetHandles().emplace_back();
		out.GetHandles().back().handle = h[i];
		out.GetHandles().back().type = tag;
		h[i] = NACL_INVALID_HANDLE;
		if (tag == NACL_DESC_SHM)
			out.GetHandles().back().size = size;
		else if (tag == NACL_DESC_HOST_IO)
			out.GetHandles().back().flags = flags;
	}

	out.GetData().assign(&desc_end[0], &desc_end[result]);
	return out;
#endif
}

std::pair<Socket, Socket> Socket::CreatePair()
{
	NaClHandle handles[2];
	if (NaClSocketPair(handles) != 0) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to create socket pair: %s", error);
	}
	Socket a, b;
	a.handle = handles[0];
	b.handle = handles[1];
	return std::make_pair(std::move(a), std::move(b));
}

static void* MapSharedMemory(OSHandleType handle, size_t size)
{
	// We don't use NaClMap here because it only supports MAP_FIXED
#ifdef _WIN32
	void *base = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
#else
	void *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
	if (base == MAP_FAILED)
		base = nullptr;
#endif

	if (base == nullptr) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to map shared memory object of size %lu: %s", size, error);
	}

	return base;
}

void SharedMemory::Close()
{
	if (handle != INVALID_HANDLE) {
#ifdef _WIN32
		UnmapViewOfFile(base);
#else
		munmap(base, size);
#endif
		NaClClose(handle);
	}
	handle = INVALID_HANDLE;
}

Desc SharedMemory::GetDesc() const
{
#ifdef __native_client__
	return handle;
#else
	Desc out;
	out.type = NACL_DESC_SHM;
	out.handle = handle;
	out.size = size;
	return out;
#endif
}
SharedMemory SharedMemory::FromDesc(const Desc& desc)
{
	SharedMemory out;
#ifdef __native_client__
	struct stat st;
	if (fstat(desc, &st) == -1) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to stat shared memory handle: %s", error);
	}
	out.size = st.st_size;
	out.handle = desc
#else
	out.size = desc.size;
	out.handle = desc.handle;
#endif
	out.base = MapSharedMemory(out.handle, out.size);
	return out;
}

SharedMemory SharedMemory::Create(size_t size)
{
	// Round size up to page size, otherwise the syscall will fail in NaCl
	size = (size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);

	OSHandleType handle = NaClCreateMemoryObject(size, 0);
	if (handle == INVALID_HANDLE) {
		char error[256];
		NaClGetLastErrorString(error, sizeof(error));
		Com_Error(ERR_DROP, "IPC: Failed to create shared memory object of size %lu: %s", size, error);
	}

	SharedMemory out;
	out.handle = handle;
	out.base = MapSharedMemory(handle, size);
	out.size = size;
	return out;
}

} // namespace IPC
