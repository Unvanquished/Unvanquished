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

#include "qcommon/qcommon.h"
#include "VirtualMachine.h"

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

namespace VM {

// Platform-specific code to load a module
static std::pair<Sys::OSHandle, IPC::Socket> InternalLoadModule(std::pair<IPC::Socket, IPC::Socket> pair, const char* const* args, bool reserve_mem, FS::File stderrRedirect = FS::File())
{
#ifdef _WIN32
	// Inherit the socket in the child process
	if (!SetHandleInformation(pair.second.GetHandle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		Sys::Drop("VM: Could not make socket inheritable: %s", Sys::Win32StrError(GetLastError()));

	// Inherit the stderr redirect in the child process
	HANDLE stderrRedirectHandle = stderrRedirect ? reinterpret_cast<HANDLE>(_get_osfhandle(fileno(stderrRedirect.GetHandle()))) : nullptr;
	if (stderrRedirect && !SetHandleInformation(stderrRedirectHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
		Sys::Drop("VM: Could not make stderr redirect inheritable: %s", Sys::Win32StrError(GetLastError()));

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
	HANDLE job = CreateJobObject(nullptr, nullptr);
	if (!job)
		Sys::Drop("VM: Could not create job object: %s", Sys::Win32StrError(GetLastError()));
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
	memset(&jeli, 0, sizeof(jeli));
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
		Sys::Drop("VM: Could not set job object information: %s", Sys::Win32StrError(GetLastError()));

	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	if (stderrRedirect) {
		startupInfo.hStdError = stderrRedirectHandle;
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
	}
	startupInfo.cb = sizeof(startupInfo);
	if (!CreateProcessW(nullptr, &wcmdline[0], nullptr, nullptr, TRUE, CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB | CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo)) {
		CloseHandle(job);
		Sys::Drop("VM: Could not create child process: %s", Sys::Win32StrError(GetLastError()));
	}

	if (!AssignProcessToJobObject(job, processInfo.hProcess)) {
		TerminateProcess(processInfo.hProcess, 0);
		CloseHandle(job);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		Sys::Drop("VM: Could not assign process to job object: %s", Sys::Win32StrError(GetLastError()));
	}

#ifndef _WIN64
	// Attempt to reserve 1GB of address space for the NaCl sandbox
	if (reserve_mem)
		VirtualAllocEx(processInfo.hProcess, nullptr, 1 << 30, MEM_RESERVE, PAGE_NOACCESS);
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
		Sys::Drop("VM: Failed to create pipe: %s", strerror(errno));

	int pid = vfork();
	if (pid == -1)
		Sys::Drop("VM: Failed to fork process: %s", strerror(errno));
	if (pid == 0) {
		// Close the other end of the pipe
		close(pipefds[0]);

		// Explicitly destroy the local socket, since destructors are not called
		close(pair.first.GetHandle());

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

		ssize_t wrote = write(pipefds[1], &errno, sizeof(int));
		Q_UNUSED(wrote);
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
		waitpid(pid, nullptr, 0);
		Sys::Drop("VM: Failed to exec: %s", strerror(err));
	}

	return std::make_pair(pid, std::move(pair.first));
#endif
}

std::pair<Sys::OSHandle, IPC::Socket> CreateNaClVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, bool debug, bool extract, int debugLoader) {
	const std::string& libPath = FS::GetLibPath();
#ifdef NACL_RUNTIME_PATH
	const char* naclPath = XSTRING(NACL_RUNTIME_PATH);
#else
	const std::string& naclPath = libPath;
#endif
	std::vector<const char*> args;
	char rootSocketRedir[32];
	std::string module, nacl_loader, irt, bootstrap, modulePath, verbosity;
	FS::File stderrRedirect;
	bool win32Force64Bit = false;

	// On Windows, even if we are running a 32-bit engine, we must use the
	// 64-bit nacl_loader if the host operating system is 64-bit.
#if defined(_WIN32) && !defined(_WIN64)
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	win32Force64Bit = systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#endif

	// Extract the nexe from the pak so that nacl_loader can load it
	module = win32Force64Bit ? name + "-x86_64.nexe" : name + "-" ARCH_STRING ".nexe";
	if (extract) {
		try {
			FS::File out = FS::HomePath::OpenWrite(module);
			if (const FS::LoadedPakInfo* pak = FS::PakPath::LocateFile(module))
				Com_Printf("Extracting VM module %s from %s...\n", module.c_str(), pak->path.c_str());
			FS::PakPath::CopyFile(module, out);
			out.Close();
		} catch (std::system_error& err) {
			Sys::Drop("VM: Failed to extract VM module %s: %s", module, err.what());
		}
		modulePath = FS::Path::Build(FS::GetHomePath(), module);
	} else
		modulePath = FS::Path::Build(libPath, module);

	// Generate command line
	Q_snprintf(rootSocketRedir, sizeof(rootSocketRedir), "%d:%d", ROOT_SOCKET_FD, (int)(intptr_t)pair.second.GetHandle());
	irt = FS::Path::Build(naclPath, win32Force64Bit ? "irt_core-x86_64.nexe" : "irt_core-" ARCH_STRING ".nexe");
	nacl_loader = FS::Path::Build(naclPath, win32Force64Bit ? "nacl_loader64" EXE_EXT : "nacl_loader" EXE_EXT);
	if (!FS::RawPath::FileExists(modulePath))
		Log::Warn("VM module file not found: %s", modulePath);
	if (!FS::RawPath::FileExists(nacl_loader))
		Log::Warn("NaCl loader not found: %s", nacl_loader);
	if (!FS::RawPath::FileExists(irt))
		Log::Warn("NaCl integrated runtime not found: %s", irt);
#ifdef __linux__
	bootstrap = FS::Path::Build(naclPath, "nacl_helper_bootstrap");
	if (!FS::RawPath::FileExists(bootstrap))
		Log::Warn("NaCl bootstrap helper not found: %s", bootstrap);
	args.push_back(bootstrap.c_str());
	args.push_back(nacl_loader.c_str());
	args.push_back("--r_debug=0xXXXXXXXXXXXXXXXX");
	args.push_back("--reserved_at_zero=0xXXXXXXXXXXXXXXXX");
#else
	Q_UNUSED(bootstrap);
	args.push_back(nacl_loader.c_str());
#endif
	if (debug) {
		args.push_back("-g");
	}

	if (debugLoader) {
		std::error_code err;
		stderrRedirect = FS::HomePath::OpenWrite(name + ".nacl_loader.log", err);
		if (err)
			Log::Warn("Couldn't open %s: %s", name + ".nacl_loader.log", err.message());
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
	args.push_back(nullptr);

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

std::pair<Sys::OSHandle, IPC::Socket> CreateNativeVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, bool debug) {
	const std::string& libPath = FS::GetLibPath();
	std::vector<const char*> args;

	std::string handleArg = std::to_string((int)(intptr_t)pair.second.GetHandle());

	std::string module = FS::Path::Build(libPath, name + "-native-exe" + EXE_EXT);
	if (debug) {
		args.push_back("/usr/bin/gdbserver");
		args.push_back("localhost:4014");
	}
	args.push_back(module.c_str());
	args.push_back(handleArg.c_str());
	args.push_back(nullptr);

	Com_Printf("Loading VM module %s...\n", module.c_str());

	return InternalLoadModule(std::move(pair), args.data(), true);
}

IPC::Socket CreateInProcessNativeVM(std::pair<IPC::Socket, IPC::Socket> pair, Str::StringRef name, VM::VMBase::InProcessInfo& inProcess) {
	std::string filename = FS::Path::Build(FS::GetLibPath(), name + "-native-dll" + DLL_EXT);

	Com_Printf("Loading VM module %s...\n", filename.c_str());

	std::string errorString;
	inProcess.sharedLib = Sys::DynamicLib::Open(filename, errorString);
	if (!inProcess.sharedLib)
		Sys::Drop("VM: Failed to load shared library VM %s: %s", filename, errorString);

	auto vmMain = inProcess.sharedLib.LoadSym<void(Sys::OSHandle)>("vmMain", errorString);
	if (!vmMain)
		Sys::Drop("VM: Could not find vmMain function in %s: %s", filename, errorString);

	Sys::OSHandle vmSocketArg = pair.second.ReleaseHandle();
	inProcess.running = true;
	try {
		inProcess.thread = std::thread([vmMain, vmSocketArg, &inProcess]() {
			vmMain(vmSocketArg);

			std::lock_guard<std::mutex> lock(inProcess.mutex);
			inProcess.running = false;
			inProcess.condition.notify_one();
		});
	} catch (std::system_error& err) {
		// Close vmSocketArg using the Socket destructor
		IPC::Socket::FromHandle(vmSocketArg);
		inProcess.running = false;
		Sys::Drop("VM: Could not create thread for VM: %s", err.what());
	}

	return std::move(pair.first);
}

uint32_t VMBase::Create()
{
	type = static_cast<vmType_t>(params.vmType.Get());

	if (type < 0 || type >= TYPE_END)
		Sys::Drop("VM: Invalid type %d", type);

	int loadStartTime = Sys_Milliseconds();

	// Free the VM if it exists
	Free();

	// Open the syscall log
	if (params.logSyscalls.Get()) {
		std::string filename = name + ".syscallLog";
		std::error_code err;
		syscallLogFile = FS::HomePath::OpenWrite(filename, err);
		if (err)
			Log::Warn("Couldn't open %s: %s", filename, err.message());
	}

	// Create the socket pair to get the handle for the root socket
	std::pair<IPC::Socket, IPC::Socket> pair = IPC::Socket::CreatePair();

	IPC::Socket rootSocket;
	if (type == TYPE_NACL || type == TYPE_NACL_LIBPATH) {
		std::tie(processHandle, rootSocket) = CreateNaClVM(std::move(pair), name, params.debug.Get(), type == TYPE_NACL, params.debugLoader.Get());
	} else if (type == TYPE_NATIVE_EXE) {
		std::tie(processHandle, rootSocket) = CreateNativeVM(std::move(pair), name, params.debug.Get());
	} else {
		rootSocket = CreateInProcessNativeVM(std::move(pair), name, inProcess);
	}
	rootChannel = IPC::Channel(std::move(rootSocket));

	if (type != TYPE_NATIVE_DLL && params.debug.Get())
		Com_Printf("Waiting for GDB connection on localhost:4014\n");

	// Only set a recieve timeout for non-debug configurations, otherwise it
	// would get triggered by breakpoints.
	if (type != TYPE_NATIVE_DLL && !params.debug.Get())
		rootChannel.SetRecvTimeout(std::chrono::seconds(2));

	// Read the ABI version from the root socket.
	// If this fails, we assume the remote process failed to start
	Util::Reader reader = rootChannel.RecvMsg();
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

	inProcess.sharedLib.Close();
	inProcess.running = false;
}

void VMBase::LogMessage(bool vmToEngine, bool start, int id)
{
	if (syscallLogFile) {
		int minor = id & 0xffff;
		int major = id >> 16;

		const char* direction = vmToEngine ? "V->E" : "E->V";
		const char* extremity = start ? "start" : "end";
		uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Sys::SteadyClock::now().time_since_epoch()).count();
		try {
			syscallLogFile.Printf("%s %s %s %s %s\n", direction, extremity, major, minor, ns);
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

	// First send a message signaling an exit to the VM
	// then delete the socket. This is needed because
	// recvmsg in NaCl doesn't return when the socket has
	// been closed.
	Util::Writer writer;
	writer.Write<uint32_t>(IPC::ID_EXIT);
	rootChannel.SendMsg(writer);

	rootChannel = IPC::Channel();

	if (type != TYPE_NATIVE_DLL) {
#ifdef _WIN32
		// Closing the job object should kill the child process
		CloseHandle(processHandle);
#else
		int status;
		if (waitpid(processHandle, &status, WNOHANG) != 0) {
			if (WIFSIGNALED(status))
				Log::Warn("VM exited with signal %d: %s\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
			else if (WIFEXITED(status))
				Log::Warn("VM exited with non-zero exit code %d\n", WEXITSTATUS(status));
		}
		kill(processHandle, SIGKILL);
		waitpid(processHandle, nullptr, 0);
#endif
		processHandle = Sys::INVALID_HANDLE;
	} else {
		FreeInProcessVM();
	}

}

} // namespace VM
