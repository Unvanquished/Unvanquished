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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../common/Log.h"
#include "VirtualMachine.h"
#include "../sys/sys_loadlib.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#undef CopyFile
#else
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#endif

// File handle for the root socket
#define ROOT_SOCKET_FD 100

// MinGW doesn't define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#ifndef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE 0x2000
#endif

// On windows use _snprintf instead of snprintf
#ifdef _WIN32
#define snprintf _snprintf
#endif

namespace VM {

// Windows equivalent of strerror
#ifdef _WIN32
static std::string Win32StrError(uint32_t error)
{
	std::string out;
	char* message;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char *>(&message), 0, NULL)) {
		out = message;
		LocalFree(message);
	} else
		out = Str::Format("Unknown error 0x%08lx", error);
	return out;
}
#endif

// Platform-specific code to load a module
static std::pair<IPC::OSHandleType, IPC::Socket> InternalLoadModule(std::pair<IPC::Socket, IPC::Socket> pair, const char* const* args, bool reserve_mem, FS::File stderrRedirect = FS::File())
{
#ifdef _WIN32
	// Inherit the socket in the child process
	if (!SetHandleInformation(pair.second.GetHandle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		Com_Error(ERR_DROP, "VM: Could not make socket inheritable: %s", Win32StrError(GetLastError()).c_str());

	// Inherit the stderr redirect in the child process
	HANDLE stderrRedirectHandle = stderrRedirect ? reinterpret_cast<HANDLE>(_get_osfhandle(fileno(stderrRedirect.GetHandle()))) : NULL;
	if (stderrRedirect && !SetHandleInformation(stderrRedirectHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		Com_Error(ERR_DROP, "VM: Could not make stderr redirect inheritable: %s", Win32StrError(GetLastError()).c_str());

	// Escape command line arguments
	std::string cmdline;
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
				cmdline.push_back(c);
			}
		}
		cmdline += "\"";
	}

	// Convert command line to UTF-16 and add a NUL terminator
	std::wstring wcmdline = Str::UTF8To16(cmdline) + L"\0";

	// Create a job object to ensure the process is terminated if the parent dies
	HANDLE job = CreateJobObject(NULL, NULL);
	if (!job)
		Com_Error(ERR_DROP, "VM: Could not create job object: %s", Win32StrError(GetLastError()).c_str());
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
	memset(&jeli, 0, sizeof(jeli));
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
		Com_Error(ERR_DROP, "VM: Could not set job object information: %s", Win32StrError(GetLastError()).c_str());

	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	if (stderrRedirect) {
		startupInfo.hStdError = stderrRedirectHandle;
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
	}
	startupInfo.cb = sizeof(startupInfo);
	if (!CreateProcessW(NULL, &wcmdline[0], NULL, NULL, TRUE, CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB | DETACHED_PROCESS, NULL, NULL, &startupInfo, &processInfo)) {
		CloseHandle(job);
		Com_Error(ERR_DROP, "VM: Could not create child process: %s", Win32StrError(GetLastError()).c_str());
	}

	if (!AssignProcessToJobObject(job, processInfo.hProcess)) {
		TerminateProcess(processInfo.hProcess, 0);
		CloseHandle(job);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		Com_Error(ERR_DROP, "VM: Could not assign process to job object: %s", Win32StrError(GetLastError()).c_str());
	}

#ifndef _WIN64
	// Attempt to reserve 1GB of address space for the NaCl sandbox
	if (reserve_mem)
		VirtualAllocEx(processInfo.hProcess, NULL, 1 << 30, MEM_RESERVE, PAGE_NOACCESS);
#endif

	ResumeThread(processInfo.hThread);
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);

	return std::make_pair(job, std::move(pair.first));
#else
	Q_UNUSED(reserve_mem);

	// Create a pipe to report errors from the child process
	int pipefds[2];
	if (pipe(pipefds) == -1 || fcntl(pipefds[1], F_SETFD, FD_CLOEXEC))
		Com_Error(ERR_DROP, "VM: Failed to create pipe: %s", strerror(errno));

	int pid = fork();
	if (pid == -1)
		Com_Error(ERR_DROP, "VM: Failed to fork process: %s", strerror(errno));
	if (pid == 0) {
		// Close the other end of the pipe
		close(pipefds[0]);

		// Explicitly destroy the local socket, since destructors are not called
		pair.first.Close();

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

		// If an stderr redirect is provided, set it now
		if (stderrRedirect)
			dup2(fileno(stderrRedirect.GetHandle()), 2);

		// Load the target executable
		char* env[] = {nullptr};
		execve(args[0], const_cast<char* const*>(args), env);

		// If the exec fails, return errno to the parent through the pipe
		write(pipefds[1], &errno, sizeof(int));
		_exit(-1);
	}

	// Try to read from the pipe to see if the child successfully exec'd
	close(pipefds[1]);
	ssize_t count;
	int err;
	while ((count = read(pipefds[0], &err, sizeof(int))) == -1) {
		if (errno != EAGAIN && errno != EINTR)
			break;
	}
	close(pipefds[0]);
	if (count) {
		waitpid(pid, NULL, 0);
		Com_Error(ERR_DROP, "VM: Failed to exec: %s", strerror(err));
	}

	return std::make_pair(pid, std::move(pair.first));
#endif
}

std::pair<IPC::OSHandleType, IPC::Socket> CreateNaClVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, bool debug, bool extract, int debugLoader) {
	// Generate command line
	const std::string& libPath = FS::GetLibPath();
	std::vector<const char*> args;
	char rootSocketRedir[32];
	std::string module, sel_ldr, irt, bootstrap, modulePath, verbosity;
	FS::File stderrRedirect;
	bool win32Force64Bit = false;

	// On Windows, even if we are running a 32-bit engine, we must use the
	// 64-bit sel_ldr if the host operating system is 64-bit.
#if defined(_WIN32) && !defined(_WIN64)
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	win32Force64Bit = systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#endif

	// Extract the nexe from the pak so that sel_ldr can load it
	module = win32Force64Bit ? name + "-x86_64.nexe" : name + "-" ARCH_STRING ".nexe";
	if (extract) {
		try {
			FS::File out = FS::HomePath::OpenWrite(module);
			FS::PakPath::CopyFile(module, out);
			out.Close();
		} catch (std::system_error& err) {
			Com_Error(ERR_DROP, "VM: Failed to extract VM module %s: %s\n", module.c_str(), err.what());
		}
		modulePath = FS::Path::Build(FS::GetHomePath(), module);
	} else
		modulePath = FS::Path::Build(libPath, module);

	snprintf(rootSocketRedir, sizeof(rootSocketRedir), "%d:%d", ROOT_SOCKET_FD, (int)(intptr_t)pair.second.GetHandle());
	irt = FS::Path::Build(libPath, win32Force64Bit ? "irt_core-x86_64.nexe" : "irt_core-" ARCH_STRING ".nexe");

	sel_ldr = FS::Path::Build(libPath, win32Force64Bit ? "sel_ldr64" EXE_EXT : "sel_ldr" EXE_EXT);

#ifdef __linux__
	bootstrap = FS::Path::Build(libPath, "nacl_helper_bootstrap");
	args.push_back(bootstrap.c_str());
	args.push_back(sel_ldr.c_str());
	args.push_back("--r_debug=0xXXXXXXXXXXXXXXXX");
	args.push_back("--reserved_at_zero=0xXXXXXXXXXXXXXXXX");
#else
	Q_UNUSED(bootstrap);
	args.push_back(sel_ldr.c_str());
#endif
	if (debug) {
		args.push_back("-g");
	}

	if (debugLoader) {
		try {
			stderrRedirect = FS::HomePath::OpenWrite(name + ".sel_ldr.log");
		} catch (std::system_error& err) {
			Log::Warn("Couldn't open %s: %s", name + ".sel_ldr.log", err.what());
		}
		verbosity = "-";
		verbosity.append(debugLoader, 'v');
		args.push_back(verbosity.c_str());
	}

	args.push_back("-B");
	args.push_back(irt.c_str());
	args.push_back("-e");
	args.push_back("-i");
	args.push_back(rootSocketRedir);
	args.push_back("--");
	args.push_back(modulePath.c_str());
	args.push_back(XSTRING(ROOT_SOCKET_FD));
	args.push_back(NULL);

	Com_Printf("Loading VM module %s...\n", module.c_str());

	if (debugLoader) {
		std::string commandLine;
		for (auto arg : args) {
			if (arg) {
				commandLine += " ";
				commandLine += arg;
			}
		}
		Com_Printf("Using loader args: %s", commandLine.c_str());
	}

	return InternalLoadModule(std::move(pair), args.data(), true, std::move(stderrRedirect));
}

std::pair<IPC::OSHandleType, IPC::Socket> CreateNativeVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, bool debug) {
	const std::string& libPath = FS::GetLibPath();
	std::vector<const char*> args;

	std::string handleArg = std::to_string((int)(intptr_t)pair.second.GetHandle());

	std::string module = FS::Path::Build(libPath, name + "-nacl-native-exe" + EXE_EXT);
	if (debug) {
		args.push_back("/usr/bin/gdbserver");
		args.push_back("localhost:4014");
	}
	args.push_back(module.c_str());
	args.push_back(handleArg.c_str());
	args.push_back(NULL);

	Com_Printf("Loading VM module %s...\n", module.c_str());

	return InternalLoadModule(std::move(pair), args.data(), true);
}

IPC::Socket CreateInProcessNativeVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, VM::VMBase::InProcessInfo& inProcess) {
	std::string filename = FS::Path::Build(FS::GetLibPath(), name + "-nacl-native-dll" + DLL_EXT);

	Com_Printf("Loading VM module %s...\n", filename.c_str());

	void* handle = Sys_LoadLibrary(filename.c_str());
	if (!handle) {
		Com_Error(ERR_DROP, "VM: Failed to load shared library VM %s: %s", filename.c_str(), Sys_LibraryError());
	}
	inProcess.sharedLibHandle = handle;

	int (*vmMain)(int, const char**) = (int (*)(int, const char**))(Sys_LoadFunction(handle, "main"));
	if (!vmMain) {
		Com_Error(ERR_DROP, "VM: Could not find main function in shared library VM %s", filename.c_str());
	}

	std::string vmSocketArg = std::to_string((int)(intptr_t)pair.second.ReleaseHandle());

	inProcess.running = true;
	inProcess.thread = std::thread([vmMain, vmSocketArg, &inProcess]() {
		const char* args[2] = {"vm", vmSocketArg.c_str()};
		vmMain(2, args);

		std::lock_guard<std::mutex> lock(inProcess.mutex);
		inProcess.running = false;
		inProcess.condition.notify_one();
	});

	return std::move(pair.first);
}

int VMBase::Create()
{
	type = static_cast<vmType_t>(params.vmType.Get());

	if (type < 0 || type >= TYPE_END)
		Com_Error(ERR_DROP, "VM: Invalid type %d", type);

	int loadStartTime = Sys_Milliseconds();

	// Free the VM if it exists
	Free();

	// Open the syscall log
	if (params.logSyscalls.Get()) {
		std::string filename = name + ".syscallLog";
		try {
			syscallLogFile = FS::RawPath::OpenWrite(filename);
		} catch (std::system_error& err) {
			Log::Warn("Couldn't open %s: %s", filename, err.what());
		}
	}

	// Create the socket pair to get the handle for ROOT_SOCKET
	std::pair<IPC::Socket, IPC::Socket> pair = IPC::Socket::CreatePair();

	IPC::Socket rootSocket;
	if (type == TYPE_NACL || type == TYPE_NACL_DEBUG || type == TYPE_NACL_LIBPATH || type == TYPE_NACL_LIBPATH_DEBUG) {
		std::tie(processHandle, rootSocket) = CreateNaClVM(std::move(pair), name, type == TYPE_NACL_DEBUG || type == TYPE_NACL_LIBPATH_DEBUG, type == TYPE_NACL || type == TYPE_NACL_DEBUG, params.debugLoader.Get());
	} else if (type == TYPE_NATIVE_EXE || type == TYPE_NATIVE_EXE_DEBUG) {
		std::tie(processHandle, rootSocket) = CreateNativeVM(std::move(pair), name, type == TYPE_NATIVE_EXE_DEBUG);
	} else {
		rootSocket = CreateInProcessNativeVM(std::move(pair), name, inProcess);
	}
	rootChannel = IPC::Channel(std::move(rootSocket));

	if (type == TYPE_NACL_DEBUG || type == TYPE_NATIVE_EXE_DEBUG || type == TYPE_NACL_LIBPATH_DEBUG)
		Com_Printf("Waiting for GDB connection on localhost:4014\n");

	// Read the ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	IPC::Reader reader = rootChannel.RecvMsg();
	Com_Printf("Loaded VM module in %d msec\n", Sys_Milliseconds() - loadStartTime);
	return reader.Read<uint32_t>();
}

void VMBase::FreeInProcessVM() {
	if (inProcess.thread.joinable()) {
		bool wait = true;
		if (inProcess.running) {
			std::unique_lock<std::mutex> lock(inProcess.mutex);
			auto status = inProcess.condition.wait_for(lock, std::chrono::milliseconds(500));
			if (status == std::cv_status::timeout) {
				wait = false;
			}
		}

		if (wait) {
			Com_Printf("Waiting for the VM thread...\n");
			inProcess.thread.join();
		} else {
			Com_Printf("The VM thread doesn't seem to stop, detaching it (bad things WILL ensue)\n");
			inProcess.thread.detach();
		}
	}

	if (inProcess.sharedLibHandle) {
		Sys_UnloadLibrary(inProcess.sharedLibHandle);
		inProcess.sharedLibHandle = nullptr;
	}

	inProcess.running = false;
}

void VMBase::LogMessage(bool vmToEngine, int id)
{
	if (syscallLogFile) {
		int minor = id & 0xffff;
		int major = id >> 16;

		const char* direction = vmToEngine ? "V -> E" : "E -> V";

		try {
			syscallLogFile.Printf("%s (%i, %i)\n", direction, major, minor);
		} catch (std::system_error& err) {
			Log::Warn("Error while writing the VM syscall log: %s", err.what());
		}
	}
}

void VMBase::Free()
{
	if (syscallLogFile) {
		std::error_code err;
		syscallLogFile.Close(err);
	}

	if (!IsActive())
		return;

	rootChannel = IPC::Channel();

	if (type == TYPE_NACL || type == TYPE_NACL_DEBUG || type == TYPE_NATIVE_EXE || type == TYPE_NATIVE_EXE_DEBUG) {
#ifdef _WIN32
		// Closing the job object should kill the child process
		CloseHandle(processHandle);
#else
		kill(processHandle, SIGKILL);
		waitpid(processHandle, NULL, 0);
#endif
		processHandle = IPC::INVALID_HANDLE;
	} else if (type == TYPE_NATIVE_DLL) {
		FreeInProcessVM();
	}

}

} // namespace VM
