#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <memory>

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/public/imc_types.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"

#include "nacl.h"

// File handle for the root socket
#define ROOT_SOCKET_FD 100

// MinGW doesn't define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#ifndef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE 0x2000
#endif

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

namespace NaCl {

bool InternalSendMsg(OSHandleType handle, const void* data, size_t len, IPCHandle* const* handles, size_t num_handles)
{
	NaClMessageHeader hdr;
	NaClIOVec iov[3];
	NaClHandle h[MSG_MAX_HANDLES];

	if (num_handles > MSG_MAX_HANDLES)
		return false;
	if (len > MSG_MAX_BYTES)
		return false;
	for (size_t i = 0; i < num_handles; i++)
		h[i] = handles[i]->handle;

#ifdef __native_client__
	hdr.iov = iov;
	hdr.iov_length = 1;
	hdr.handles = h;
	hdr.handle_count = num_handles;
	hdr.flags = 0;
	iov[0].base = const_cast<void*>(data);
	iov[0].length = len;
	return NaClSendDatagram(handle, &hdr, 0) == (int)len;
#else
	size_t desc_bytes = 0;
	std::unique_ptr<char[]> desc_buffer;
	if (num_handles != 0) {
		for (size_t i = 0; i < num_handles; i++) {
			// tag: 1 byte
			// flags: 4 bytes
			// size: 8 bytes (only for SHM)
			desc_bytes++;
			desc_bytes += sizeof(uint32_t);
			if (handles[i]->type == IPCHandle::TYPE_SHM)
				desc_bytes += sizeof(uint64_t);
			else if (handles[i]->type == IPCHandle::TYPE_FILE)
				desc_bytes += sizeof(int32_t);
		}
		// Add 1 byte end tag and round to 16 bytes
		desc_bytes = (desc_bytes + 1 + 0xf) & ~0xf;

		desc_buffer.reset(new char[desc_bytes]);
		char* desc_buffer_ptr = &desc_buffer[0];
		for (size_t i = 0; i < num_handles; i++) {
			if (handles[i]->type == IPCHandle::TYPE_SOCKET) {
				*desc_buffer_ptr++ = NACL_DESC_TRANSFERABLE_DATA_SOCKET;
				memset(desc_buffer_ptr, 0, sizeof(uint32_t));
				desc_buffer_ptr += sizeof(uint32_t);
			} if (handles[i]->type == IPCHandle::TYPE_FILE) {
				*desc_buffer_ptr++ = NACL_DESC_HOST_IO;
				memset(desc_buffer_ptr, 0, sizeof(uint32_t));
				desc_buffer_ptr += sizeof(uint32_t);
				memcpy(desc_buffer_ptr, &handles[i]->flags, sizeof(int32_t));
				desc_buffer_ptr += sizeof(int32_t);
			} else {
				*desc_buffer_ptr++ = NACL_DESC_SHM;
				memset(desc_buffer_ptr, 0, sizeof(uint32_t));
				desc_buffer_ptr += sizeof(uint32_t);
				memcpy(desc_buffer_ptr, &handles[i]->size, sizeof(uint64_t));
				desc_buffer_ptr += sizeof(uint64_t);
			}
		}
		*desc_buffer_ptr++ = NACL_DESC_TYPE_END_TAG;

		// Clear any padding bytes to avoid information leak
		std::fill(desc_buffer_ptr, &desc_buffer[desc_bytes], 0);
	}

	NaClInternalHeader internal_hdr = {{NACL_HANDLE_TRANSFER_PROTOCOL, static_cast<uint32_t>(desc_bytes)}, {}};
	hdr.iov = iov;
	hdr.iov_length = 3;
	hdr.handles = h;
	hdr.handle_count = num_handles;
	hdr.flags = 0;
	iov[0].base = &internal_hdr;
	iov[0].length = sizeof(NaClInternalHeader);
	if (desc_bytes != 0)
		iov[1].base = &desc_buffer[0];
	iov[1].length = desc_bytes;
	iov[2].base = const_cast<void*>(data);
	iov[2].length = len;

	return NaClSendDatagram(handle, &hdr, 0) == (int)(len + desc_bytes + sizeof(NaClInternalHeader));
#endif
}

bool InternalRecvMsg(OSHandleType handle, std::vector<char>& buffer, std::vector<IPCHandle>& handles)
{
	NaClMessageHeader hdr;
	NaClIOVec iov[2];
	NaClHandle h[NACL_ABI_IMC_DESC_MAX];
	std::unique_ptr<char[]> recv_buffer(new char[NACL_ABI_IMC_BYTES_MAX]);

	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++)
		h[i] = NACL_INVALID_HANDLE;
	handles.clear();
	buffer.clear();

#ifdef __native_client__
	hdr.iov = iov;
	hdr.iov_length = 1;
	hdr.handles = h;
	hdr.handle_count = NACL_ABI_IMC_DESC_MAX;
	hdr.flags = 0;
	iov[0].base = recv_buffer.get();
	iov[0].length = NACL_ABI_IMC_BYTES_MAX;

	int result = NaClReceiveDatagram(handle, &hdr, 0);
	if (result < 0)
		return false;

	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
		if (h[i] != NACL_INVALID_HANDLE) {
			handles.emplace_back();
			handles.back().handle = h[i];
		}
	}

	buffer.insert(buffer.begin(), &recv_buffer[0], &recv_buffer[result]);
	return true;
#else
	NaClInternalHeader internal_hdr;
	hdr.iov = iov;
	hdr.iov_length = 2;
	hdr.handles = h;
	hdr.handle_count = NACL_ABI_IMC_DESC_MAX;
	hdr.flags = 0;
	iov[0].base = &internal_hdr;
	iov[0].length = sizeof(NaClInternalHeader);
	iov[1].base = &recv_buffer[0];
	iov[1].length = NACL_ABI_IMC_BYTES_MAX;
	int result = NaClReceiveDatagram(handle, &hdr, 0);

	if (result < (int)sizeof(NaClInternalHeader))
		return false;
	result -= sizeof(NaClInternalHeader);

	if (internal_hdr.h.xfer_protocol_version != NACL_HANDLE_TRANSFER_PROTOCOL || result < (int)internal_hdr.h.descriptor_data_bytes)
		return false;
	result -= internal_hdr.h.descriptor_data_bytes;

	char* desc_ptr = &recv_buffer[0];
	char* desc_end = desc_ptr + internal_hdr.h.descriptor_data_bytes;
	size_t handle_index = 0;
	while (desc_ptr < desc_end) {
		int tag = 0xff & *desc_ptr++;
		if (tag == NACL_DESC_TYPE_END_TAG)
			break;
		if (tag != NACL_DESC_SHM && tag != NACL_DESC_TRANSFERABLE_DATA_SOCKET && tag != NACL_DESC_HOST_IO)
			goto fail;

		// Ignore flags
		if (desc_end - desc_ptr < (int)sizeof(uint32_t))
			goto fail;
		desc_ptr += sizeof(uint32_t);

		uint64_t size;
		int32_t flags;
		if (tag == NACL_DESC_SHM) {
			if (desc_end - desc_ptr < (int)sizeof(uint64_t))
				goto fail;
			memcpy(&size, desc_ptr, sizeof(uint64_t));
			desc_ptr += sizeof(uint64_t);
		} else if (tag == NACL_DESC_HOST_IO) {
			if (desc_end - desc_ptr < (int)sizeof(int32_t))
				goto fail;
			memcpy(&flags, desc_ptr, sizeof(int32_t));
			desc_ptr += sizeof(int32_t);
		}

		size_t i = handle_index++;
		handles.emplace_back();
		handles.back().handle = h[i];
		h[i] = NACL_INVALID_HANDLE;
		if (tag == NACL_DESC_TRANSFERABLE_DATA_SOCKET)
			handles.back().type = IPCHandle::TYPE_SOCKET;
		else if (tag == NACL_DESC_HOST_IO) {
			handles.back().type = IPCHandle::TYPE_FILE;
			handles.back().flags = flags;
		} else {
			handles.back().type = IPCHandle::TYPE_SHM;
			handles.back().size = size;
		}
	}

	buffer.insert(buffer.begin(), &desc_end[0], &desc_end[result]);
	return true;

fail:
	// If an error occurred, make sure all resources are freed
	for (size_t i = 0; i < NACL_ABI_IMC_DESC_MAX; i++) {
		if (h[i] != NACL_INVALID_HANDLE)
			NaClClose(h[i]);
	}
	handles.clear();

	return false;
#endif
}

void IPCHandle::Close()
{
	if (handle != INVALID_HANDLE)
		NaClClose(handle);
}

bool IPCHandle::SendMsg(const void* data, size_t len) const
{
	return InternalSendMsg(handle, data, len, NULL, 0);
}

bool IPCHandle::RecvMsg(std::vector<char>& buffer) const
{
	std::vector<IPCHandle> handles;
	return InternalRecvMsg(handle, buffer, handles);
}

// We don't use NaClMap here because it only supports MAP_FIXED
SharedMemoryPtr IPCHandle::Map() const
{
	// Get the size of the memory region from the NaCl runtime
#ifdef __native_client__
	struct stat st;
	if (fstat(handle, &st) != 0)
		return SharedMemoryPtr();
	size_t size = st.st_size;
#endif

#ifdef _WIN32
	void *result = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
#else
	void *result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
	if (result == MAP_FAILED)
		result = NULL;
#endif

	SharedMemoryPtr out;
	out.addr = result;
	out.size = size;
	return out;
}

void SharedMemoryPtr::Close()
{
	if (!addr)
		return;
	addr = nullptr;

#ifdef _WIN32
	UnmapViewOfFile(addr);
#else
	munmap(addr, size);
#endif
}

bool SocketPair(IPCHandle& first, IPCHandle& second)
{
	NaClHandle handles[2];
	if (NaClSocketPair(handles) != 0)
		return false;
	first.Close();
	second.Close();
	first.handle = handles[0];
	second.handle = handles[1];
#ifndef __native_client__
	first.type = second.type = IPCHandle::TYPE_SOCKET;
#endif
	return true;
}

IPCHandle CreateSharedMemory(size_t size)
{
	// Round size up to page size, otherwise the syscall will fail in NaCl
	size = (size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);

	OSHandleType h = NaClCreateMemoryObject(size, 0);
	if (h == NACL_INVALID_HANDLE)
		return IPCHandle();
	IPCHandle out;
	out.handle = h;
#ifndef __native_client__
	out.type = IPCHandle::TYPE_SHM;
	out.size = size;
#endif
	return out;
}

IPCHandle WrapFileHandle(OSHandleType handle, FileOpenMode mode)
{
	IPCHandle out;
	out.handle = handle;
#ifndef __native_client__
	out.type = IPCHandle::TYPE_FILE;
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

bool RootSocket::SendMsg(const void* data, size_t len, IPCHandle* const* handles, size_t num_handles) const
{
	return InternalSendMsg(handle, data, len, handles, num_handles);
}

bool RootSocket::RecvMsg(std::vector<char>& buffer, std::vector<IPCHandle>& handles) const
{
	return InternalRecvMsg(handle, buffer, handles);
}

// Don't include host-only definitions in NaCl build
#ifndef __native_client__

void Module::Close()
{
	if (process_handle == INVALID_HANDLE)
		return;

	NaClClose(root_socket);
#ifdef _WIN32
	// Closing the job object should kill the child process
	CloseHandle(process_handle);
#else
	kill(process_handle, SIGKILL);
	waitpid(process_handle, NULL, 0);
#endif
}

Module InternalLoadModule(NaClHandle* pair, const char* const* args, const char* const* env, bool reserve_mem)
{
#ifdef _WIN32
	if (!SetHandleInformation(pair[1], HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
		NaClClose(pair[0]);
		NaClClose(pair[1]);
		return Module();
	}

	// Escape command line arguments
	std::string cmdline = "";
	for (int i = 0; args[i]; i++) {
		if (i != 0)
			cmdline += " ";

		// Enclose parameter in quotes
		cmdline += "\"";
		int num_slashes = 0;
		for (int j = 0; true; j++) {
			char c = args[i][j];

			if (c == '\\')
				num_slashes++;
			else if (c == '\"' || c == '\0') {
				// Backlashes before a quote must be escaped
				for (int k = 0; k < num_slashes; k++)
					cmdline += "\\\\";
				num_slashes = 0;
				if (c == '\"')
					cmdline += "\\\"";
				else
					break;
			} else {
				// Backlashes before any other character must not be escaped
				for (int k = 0; k < num_slashes; k++)
					cmdline += "\\";
				num_slashes = 0;
			}
		}
		cmdline += "\"";
	}

	// Build environment data
	std::vector<char> env_data;
	while (*env) {
		env_data.insert(env_data.begin(), *env, *env + strlen(*env) + 1);
		env++;
	}
	env_data.push_back('\0');

	// Create a job object to ensure the process is terminated if the parent dies
	OSHandleType job = CreateJobObject(NULL, NULL);
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
	memset(&jeli, 0, sizeof(jeli));
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
		NaClClose(pair[0]);
		NaClClose(pair[1]);
		return Module();
	}

	STARTUPINFOA startup_info;
	PROCESS_INFORMATION process_information;
	memset(&startup_info, 0, sizeof(startup_info));
	startup_info.cb = sizeof(startup_info);
	if (!CreateProcessA(NULL, const_cast<char*>(cmdline.c_str()), NULL, NULL, TRUE, CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB | DETACHED_PROCESS, env_data.data(), NULL, &startup_info, &process_information)) {
		NaClClose(pair[0]);
		NaClClose(pair[1]);
		return Module();
	}

	if (!AssignProcessToJobObject(job, process_information.hProcess)) {
		TerminateProcess(process_information.hProcess, 0);
		CloseHandle(process_information.hThread);
		CloseHandle(process_information.hProcess);
		NaClClose(pair[0]);
		NaClClose(pair[1]);
		return Module();
	}

#ifndef _WIN64
	// Attempt to reserve 1GB of address space for the NaCl sandbox
	if (reserve_mem)
		VirtualAllocEx(process_information.hProcess, NULL, 1 << 30, MEM_RESERVE, PAGE_NOACCESS);
#endif

	ResumeThread(process_information.hThread);
	NaClClose(pair[1]);
	CloseHandle(process_information.hThread);
	CloseHandle(process_information.hProcess);

	Module out;
	out.process_handle = job;
	out.root_socket = pair[0];
	return out;
#else
	int pid = fork();
	if (pid < 0) {
		NaClClose(pair[0]);
		NaClClose(pair[1]);
		return Module();
	}
	if (pid == 0) {
		NaClClose(pair[0]);

		// This seems to be required, otherwise killing the child process will
		// also kill the parent process.
		setsid();

#ifdef __linux__
		// Kill child process if the parent dies. This avoid left-over processes
		// when using the debug stub. Sadly it is Linux-only.
		prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif

		// Close standard input/output descriptors
		close(0);
		close(1);
		close(2);

		execve(args[0], const_cast<char* const*>(args), const_cast<char* const*>(env));

		// Abort if exec failed, the parent should notice a socket error when
		// trying to use the root socket.
		_exit(-1);
	}

	NaClClose(pair[1]);

	Module out;
	out.process_handle = pid;
	out.root_socket = pair[0];
	return out;
#endif
}

Module LoadNaClModule(const char* module, const char* sel_ldr, const char* irt, const char* bootstrap)
{
	NaClHandle pair[2];
	if (NaClSocketPair(pair))
		return Module();

	char root_sock_redir[32];
	snprintf(root_sock_redir, sizeof(root_sock_redir), "%d:%d", ROOT_SOCKET_FD, (int)pair[1]);
	char root_sock_handle[32];
	snprintf(root_sock_handle, sizeof(root_sock_handle), "NACLENV_ROOT_SOCKET=%d", ROOT_SOCKET_FD);
	const char* args[] = {sel_ldr, "-B", irt, "-i", root_sock_redir, "-f", module, NULL};
	const char* args_bootstrap[] = {bootstrap, sel_ldr, "--r_debug=0xXXXXXXXXXXXXXXXX", "--reserved_at_zero=0xXXXXXXXXXXXXXXXX", "-B", irt, "-i", root_sock_redir, "-f", module, NULL};
	const char* env[] = {root_sock_handle, NULL};
	return InternalLoadModule(pair, bootstrap ? args_bootstrap : args, env, true);
}

Module LoadNaClModuleDebug(const char* module, const char* sel_ldr, const char* irt, const char* bootstrap)
{
	NaClHandle pair[2];
	if (NaClSocketPair(pair))
		return Module();

	char root_sock_redir[32];
	snprintf(root_sock_redir, sizeof(root_sock_redir), "%d:%d", ROOT_SOCKET_FD, (int)pair[1]);
	char root_sock_handle[32];
	snprintf(root_sock_handle, sizeof(root_sock_handle), "NACLENV_ROOT_SOCKET=%d", ROOT_SOCKET_FD);
	const char* args[] = {sel_ldr, "-B", irt, "-i", root_sock_redir, "-f", module, NULL};
	const char* args_bootstrap[] = {bootstrap, sel_ldr, "--r_debug=0xXXXXXXXXXXXXXXXX", "--reserved_at_zero=0xXXXXXXXXXXXXXXXX", "-g", "-B", irt, "-i", root_sock_redir, "-f", module, NULL};
	const char* env[] = {root_sock_handle, NULL};
	return InternalLoadModule(pair, bootstrap ? args_bootstrap : args, env, true);
}

Module LoadNativeModule(const char* module)
{
	NaClHandle pair[2];
	if (NaClSocketPair(pair))
		return Module();

	char root_sock_handle[32];
	snprintf(root_sock_handle, sizeof(root_sock_handle), "ROOT_SOCKET=%d", (int)pair[1]);
	const char* args[] = {module, NULL};
	const char* env[] = {root_sock_handle, NULL};
	return InternalLoadModule(pair, args, env, false);
}

#endif

static NaClHandle GetRootSocketHandle()
{
	const char* socket = getenv("ROOT_SOCKET");
	if (!socket)
		return INVALID_HANDLE;

	char* end;
	NaClHandle h = (NaClHandle)strtol(socket, &end, 10);
	if (socket == end || *end != '\0')
		return INVALID_HANDLE;

	return h;
}

RootSocket GetRootSocket()
{
	static NaClHandle h = GetRootSocketHandle();

	RootSocket out;
	out.handle = h;
	return out;
}

} // namespace NaCl
