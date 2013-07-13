#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <vector>

#ifndef NACL_H_
#define NACL_H_

namespace NaCl {

// Operating system handle type
#ifdef _WIN32
// HANDLE is defined as void* in windows.h
typedef void* OSHandleType;
static const OSHandleType INVALID_HANDLE = NULL;
#else
typedef int OSHandleType;
static const OSHandleType INVALID_HANDLE = -1;
#endif

// Maximum number of bytes that can be sent in a message
static const size_t MSG_MAX_BYTES = 128 << 10;

// Maximum number of handles that can be sent in a message
static const size_t MSG_MAX_HANDLES = 8;

// File modes
enum FileOpenMode {
	MODE_READ,
	MODE_WRITE,
	MODE_RW,
	MODE_WRITE_APPEND,
	MODE_RW_APPEND
};

// Pointer to a shared-memory region
class SharedMemoryPtr {
public:
	SharedMemoryPtr()
		: addr(nullptr) {}
	SharedMemoryPtr(SharedMemoryPtr&& other)
		: addr(other.addr), size(other.size)
	{
		other.addr = nullptr;
	}
	SharedMemoryPtr& operator=(SharedMemoryPtr&& other)
	{
		std::swap(addr, other.addr);
		std::swap(size, other.size);
	}
	SharedMemoryPtr(std::nullptr_t)
		: addr(nullptr) {}
	SharedMemoryPtr& operator=(std::nullptr_t)
	{
		Unmap();
		addr = nullptr;
	}

	// Automatically release the memory mapping on destruction
	~SharedMemoryPtr()
	{
		Unmap();
	}

	explicit operator bool() const
	{
		return addr != nullptr;
	}

	// Get the mapping parameters
	void* Get() const
	{
		return addr;
	}
	size_t GetSize() const
	{
		return size;
	}

private:
	friend class IPCHandle;

	void Unmap();

	void* addr;
	size_t size;
};

// IPC object handle, which can refer to either:
// - A message-based socket
// - A shared-memory region
// - An open file
class IPCHandle {
public:
	// Close the handle
	~IPCHandle();

	// Release ownership of the underlying OS handle so that it isn't closed
	// when the IPCHandle object is destroyed.
	OSHandleType ReleaseHandle()
	{
		OSHandleType out = handle;
		handle = INVALID_HANDLE;
		return out;
	}

	// Send a message through the socket
	// Returns the number of bytes sent or -1 on error
	int SendMsg(const void* data, size_t len);

	// Recieve a message from the socket, will block until a message arrives.
	// Returns the number of bytes sent or -1 on error
	int RecvMsg(std::vector<char>& buffer);

	// Map a shared memory region into memory
	// Returns a pointer to the memory mapping or NULL on error
	// The mapping remains valid even after the handle is closed
	SharedMemoryPtr Map();

private:
	OSHandleType handle;

	// Handles are non-copyable
	IPCHandle() {}
	IPCHandle(const IPCHandle&);
	IPCHandle& operator=(const IPCHandle&);

	friend bool InternalSendMsg(OSHandleType handle, const void* data, size_t len, IPCHandle* const* handles, size_t num_handles);
	friend bool InternalRecvMsg(OSHandleType handle, std::vector<char>& buffer, std::vector<std::unique_ptr<IPCHandle>>& handles);
	friend bool SocketPair(std::unique_ptr<IPCHandle>& first, std::unique_ptr<IPCHandle>& second);
	friend std::unique_ptr<IPCHandle> CreateSharedMemory(size_t size);
	friend std::unique_ptr<IPCHandle> WrapFileHandle(OSHandleType handle, FileOpenMode mode);

	// Information only required on the host side for handle transfer protocol
#ifndef __native_client__
	enum {
		TYPE_SOCKET,
		TYPE_SHM,
		TYPE_FILE
	};
	int type;
	union {
		uint64_t size;
		int32_t flags;
	};
#endif
};

// Create a pair of sockets which are linked to each other.
// Returns false on error
bool SocketPair(std::unique_ptr<IPCHandle>& first, std::unique_ptr<IPCHandle>& second);

// Allocate a shared memory region of the specified size.
std::unique_ptr<IPCHandle> CreateSharedMemory(size_t size);

// Wrap an open file handle in an IPCHandle
std::unique_ptr<IPCHandle> WrapFileHandle(OSHandleType handle, FileOpenMode mode);

// Host-only definitions
#ifndef __native_client__

class Module {
public:
	// Close the module and kill the NaCl process
	~Module();

	// Send a message through the root socket, optionally with a some IPC handles.
	// Returns the number of bytes sent or -1 on error
	int SendMsg(const void* data, size_t len, IPCHandle* const* handles, size_t num_handles);

	// Recieve a message from the root socket, will block until a message arrives.
	// Up to num_handles handles are retrieved from the message, any further handles are discarded.
	// If there are less than num_handles handles, the remaining entries are filled with NULL pointers.
	// Returns the number of bytes recieved or -1 on error
	int RecvMsg(std::vector<char>& buffer, std::vector<std::unique_ptr<IPCHandle>>& handles);

	// Overloads for sending and recieving data without any handles
	int SendMsg(const void* data, size_t len)
	{
		return SendMsg(data, len, NULL, 0);
	}
	int RecvMsg(std::vector<char>& buffer)
	{
		std::vector<std::unique_ptr<IPCHandle>> handles;
		return RecvMsg(buffer, handles);
	}

private:
	OSHandleType root_socket;
	OSHandleType process_handle;

	// Modules are non-copyable
	Module() {}
	Module(const Module&);
	Module& operator=(const Module&);

	friend std::unique_ptr<Module> InternalLoadModule(const char* const*, OSHandleType*);
};

// Load a NaCl module
std::unique_ptr<Module> LoadNaClModule(const char* module, const char* sel_ldr, const char* irt);

// Load a NaCl module using the integrated debugger. The module will wait for a
// debugger to attach on localhost:4014 before starting.
std::unique_ptr<Module> LoadNaClModuleDebug(const char* module, const char* sel_ldr, const char* irt);

// Load a native module
std::unique_ptr<Module> LoadNativeModule(const char* module);

#endif

// Root socket of a module, created by the parent process. This is the only
// socket which can transfer IPCHandles in messages.
class RootSocket {
public:
	// Note that the socket is not closed in the destructor. This is done to
	// allow the socket to be re-created at any time using GetRootSocket.
	~RootSocket() {}

	// Root socket operations, see the descriptions in Module
	int SendMsg(const void* data, size_t len, IPCHandle* const* handles, size_t num_handles);
	int RecvMsg(std::vector<char>& buffer, std::vector<std::unique_ptr<IPCHandle>>& handles);
	inline int SendMsg(const void* data, size_t len)
	{
		return SendMsg(data, len, NULL, 0);
	}
	inline int RecvMsg(std::vector<char>& buffer)
	{
		std::vector<std::unique_ptr<IPCHandle>> handles;
		return RecvMsg(buffer, handles);
	}

private:
	OSHandleType handle;

	// The root socket is non-copyable
	RootSocket() {}
	RootSocket(const RootSocket&);
	RootSocket& operator=(const RootSocket&);

	friend std::unique_ptr<RootSocket> GetRootSocket(const char* arg);
};

// Create the root socket from the module's command line arguments. The first
// argument contains the root socket handle and must be passed to this function.
std::unique_ptr<RootSocket> GetRootSocket(const char* arg);

} // namespace NaCl

#endif // NACL_H_
