/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

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

===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../common/Log.h"
#include "CommandSystem.h"
#include "System.h"
#include <thread>
#include <vector>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <signal.h>
#endif
#if defined(_WIN32) || (!defined(DEDICATED) && !defined(BUILD_TTY_CLIENT))
#include <SDL.h>
#endif

namespace Sys {

#ifdef _WIN32
SteadyClock::time_point SteadyClock::now() NOEXCEPT
{
	// Determine performance counter frequency
	static double nanosec_per_tic = 0.0;
	if (nanosec_per_tic == 0.0) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		nanosec_per_tic = 1000000000.0 / freq.QuadPart;
	}

	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	return time_point(duration(rep(nanosec_per_tic * time.QuadPart)));
}
#endif

SteadyClock::time_point SleepUntil(SteadyClock::time_point time)
{
	// Early exit if we are already past the deadline
	auto now = SteadyClock::now();
	if (now >= time) {
		// We were already past our deadline, which means that the previous frame
		// ran for too long. Use the current time as the base for the next frame.
		return now;
	}

#ifdef _WIN32
	// Load ntdll.dll functions
	static NTSTATUS WINAPI (*pNtQueryTimerResolution)(ULONG* min_resolution, ULONG* max_resolution, ULONG* current_resolution);
	static NTSTATUS WINAPI (*pNtSetTimerResolution)(ULONG resolution, BOOLEAN set_resolution, ULONG* current_resolution);
	static NTSTATUS WINAPI (*pNtDelayExecution)(BOOLEAN alertable, const LARGE_INTEGER *timeout);
	if (!pNtQueryTimerResolution) {
		std::string errorString;
		static DynamicLib ntdll = DynamicLib::Load("ntdll.dll", errorString);
		if (!ntdll)
			Sys::Error("Failed to load ntdll.dll: %s", errorString);
		pNtQueryTimerResolution = ntdll.LoadSym<decltype(pNtQueryTimerResolution)>("NtQueryTimerResolution", errorString);
		if (!pNtQueryTimerResolution)
			Sys::Error("Failed to load NtQueryTimerResolution from ntdll.dll: %s", errorString);
		pNtSetTimerResolution = ntdll.LoadSym<decltype(pNtSetTimerResolution)>("NtSetTimerResolution", errorString);
		if (!pNtSetTimerResolution)
			Sys::Error("Failed to load NtSetTimerResolution from ntdll.dll: %s", errorString);
		pNtDelayExecution = ntdll.LoadSym<decltype(pNtDelayExecution)>("NtDelayExecution", errorString);
		if (!pNtDelayExecution)
			Sys::Error("Failed to load NtDelayExecution from ntdll.dll: %s", errorString);
	}

	// Determine the maximum available timer resolution
	static ULONG maxRes = 0;
	ULONG minRes, curRes;
	if (maxRes == 0) {
		if (pNtQueryTimerResolution(&minRes, &maxRes, &curRes) != 0)
			maxRes = 10000; // Default to 1ms
	}

	// Increase the system timer resolution for the duration of the sleep
	pNtSetTimerResolution(maxRes, TRUE, &curRes);

	// Convert to NT units of 100ns
	typedef std::chrono::duration<int64_t, std::ratio<1, 10000000>> NTDuration;
	auto remaining = std::chrono::duration_cast<NTDuration>(time - now);

	// Store the delay as a negative number to indicate a relative sleep
	LARGE_INTEGER duration;
	duration.QuadPart = -remaining.count();
	pNtDelayExecution(FALSE, &duration);

	// Restore timer resolution after sleeping
	pNtSetTimerResolution(maxRes, FALSE, &curRes);
#else
	std::this_thread::sleep_for(time - now);
#endif

	// We may have overslept, so use the target time rather than the
	// current time as the base for the next frame. That way we ensure
	// that the frame rate remains consistent.
	return time;
}

// Common code for fatal and non-fatal exit
static void Shutdown(Str::StringRef message)
{
	CL_Shutdown();
	SV_Shutdown(message.empty() ? "Server quit" : va("Server fatal crashed: %s\n", message));
	Com_Shutdown(false);

	// Always run SDL_Quit, because it restore system resolution and gamma.
#if defined(_WIN32) || (!defined(DEDICATED) && !defined(BUILD_TTY_CLIENT))
	SDL_Quit();
#endif

	// TODO: shutdown console
}

void Quit()
{
	Shutdown("");

	exit(0);
}

void Drop(Str::StringRef message)
{
	throw DropErr(message.c_str());
}

void Error(Str::StringRef message)
{
	static bool errorEntered = false;
	if (errorEntered)
		_exit(-1);
	errorEntered = true;

	Log::Error(message);

	Shutdown(message);

#if defined(_WIN32) || (!defined(DEDICATED) && !defined(BUILD_TTY_CLIENT))
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Unvanquished", message.c_str(), nullptr);
#endif

	exit(1);
}

#ifdef _WIN32
std::string Win32StrError(uint32_t error)
{
	std::string out;
	char* message;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), reinterpret_cast<char *>(&message), 0, NULL)) {
		out = message;
		LocalFree(message);
	} else
		out = Str::Format("Unknown error 0x%08lx", error);
	return out;
}
#endif

void DynamicLib::Close()
{
	if (!handle)
		return;

#ifdef _WIN32
	FreeLibrary(static_cast<HMODULE>(handle));
#else
	dlclose(handle);
#endif
}

DynamicLib DynamicLib::Load(Str::StringRef filename, std::string& errorString)
{
#ifdef _WIN32
	void* handle = LoadLibraryW(Str::UTF8To16(filename).c_str());
	if (!handle)
		errorString = Win32StrError(GetLastError());
#else
	void* handle = dlopen(filename.c_str(), RTLD_NOW);
	if (!handle)
		errorString = dlerror();
#endif

	DynamicLib out;
	out.handle = handle;
	return out;
}

void* DynamicLib::InternalLoadSym(Str::StringRef sym, std::string& errorString)
{
#ifdef _WIN32
	void* p = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(GetProcAddress(static_cast<HMODULE>(handle), sym.c_str())));
	if (!p)
		errorString = Win32StrError(GetLastError());
	return p;
#else
	void* p = dlsym(handle, sym.c_str());
	if (!p)
		errorString = dlerror();
	return p;
#endif
}

// Command line arguments
struct cmdlineArgs_t {
	const char* homepath;
	std::vector<const char*> paths;
	std::vector<std::string> commands;
};
static cmdlineArgs_t cmdlineArgs;

// Parse the command line arguments and put the results in cmdlineArgs
#ifdef DEDICATED
#define UNVANQUISHED_URL ""
#else
#define UNVANQUISHED_URL " [unv://ADDRESS[:PORT]]"
#endif
static void ParseCmdline(int argc, char** argv)
{
	bool foundCommands = false;
	for (int i = 1; i < argc; i++) {
		// A + indicate the start of a command that should be run on startup
		if (argv[i][0] == '+') {
			foundCommands = true;
			cmdlineArgs.commands.push_back(argv[i] + 1);
			continue;
		}

		// Anything after a + is a parameter for that command
		if (foundCommands) {
			cmdlineArgs.commands.back().push_back(' ');
			cmdlineArgs.commands.back().append(Cmd::Escape(argv[i]));
			continue;
		}

		if (!strcmp(argv[i], "--help")) {
			fprintf(stderr, PRODUCT_NAME " " PRODUCT_VERSION "\n"
			                "Usage: %s" UNVANQUISHED_URL " [OPTION] [+COMMAND]...\n",
			                argv[0]);
			exit(0);
		} else if (!strcmp(argv[i], "--version")) {
			fprintf(stderr, PRODUCT_NAME " " PRODUCT_VERSION "\n");
			exit(0);
#ifdef MACOS_X
		} else if (!strncmp(argv[i], "-psn", 4) {
			// Ignore -psn parameters added by OSX
#endif
		} else if (!strcmp(argv[i], "-path")) {
			if (i == argc - 1 || argv[i + 1][0] == '+') {
				Log::Warn("Missing argument for -path");
				continue;
			}
			cmdlineArgs.paths.push_back(argv[i + 1]);
			i++;
		} else if (!strcmp(argv[i], "-homepath")) {
			if (i == argc - 1 || argv[i + 1][0] == '+') {
				Log::Warn("Missing argument for -homepath");
				continue;
			}
			cmdlineArgs.paths.push_back(argv[i + 1]);
			i++;
		} else {
			Log::Warn("Ignoring unrecognized parameter \"%s\"", argv[i]);
			continue;
		}
	}
}

// Setup crash handling
#ifdef _WIN32
static const char *WindowsExceptionString(DWORD code)
{
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:
		return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:
		return "Breakpoint was encountered";
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		return "Float: Denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		return "Float: Divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:
		return "Float: Inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:
		return "Float: Invalid operation";
	case EXCEPTION_FLT_OVERFLOW:
		return "Float: Overflow";
	case EXCEPTION_FLT_STACK_CHECK:
		return "Float: Stack check";
	case EXCEPTION_FLT_UNDERFLOW:
		return "Float: Underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:
		return "Page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return "Integer: Divide by zero";
	case EXCEPTION_INT_OVERFLOW:
		return "Integer: Overflow";
	case EXCEPTION_INVALID_DISPOSITION:
		return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:
		return "Privileged instruction";
	case EXCEPTION_SINGLE_STEP:
		return "Single step";
	case EXCEPTION_STACK_OVERFLOW:
		return "Stack overflow";
	default:
		return "Unknown exception";
	}
}
static LONG CALLBACK CrashHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	static bool enteredHandler = false;
	if (enteredHandler)
		_exit(-1);
	enteredHandler = true;

	// TODO: backtrace

	Sys::Error("Caught exception 0x%lx: %s", ExceptionInfo->ExceptionRecord->ExceptionCode, WindowsExceptionString(ExceptionInfo->ExceptionRecord->ExceptionCode));
}
static void SetupCrashHandler()
{
	AddVectoredExceptionHandler(1, CrashHandler);
}
#else
static void CrashHandler(int sig)
{
	static bool enteredHandler = false;
	if (enteredHandler)
		_exit(-1);
	enteredHandler = true;

	// TODO: backtrace

	Sys::Error("Caught signal %d: %s", sig, strsignal(sig));
}
static void SetupCrashHandler()
{
	struct sigaction sa;
	sa.sa_flags = 0;
	sa.sa_handler = CrashHandler;
	sigemptyset(&sa.sa_mask);
	for (int sig: {SIGILL, SIGFPE, SIGSEGV, SIGABRT, SIGBUS, SIGTRAP})
		sigaction(sig, &sa, nullptr);
}
#endif

#ifndef _WIN32
static void SignalThread()
{
	// Wait for the signals we are interested in
	sigset_t sigset;
	sigemptyset(&sigset);
	for (int sig: {SIGTERM, SIGINT, SIGQUIT, SIGPIPE, SIGHUP})
		sigaddset(&sigset, sig);
	int sig;
	sigwait(&sigset, &sig);

	// Queue a quit command to be executed next frame
	Cmd::BufferCommandText("quit");

	// Now let the thread exit, which will cause all future instances of these
	// signals to be ignored since they are blocked in all threads.
}
static void StartSignalThread()
{
	// Block all signals we are interested in. This will cause the threads to be
	// blocked in all threads since they inherit the signal mask.
	sigset_t sigset;
	sigemptyset(&sigset);
	for (int sig: {SIGTERM, SIGINT, SIGQUIT, SIGPIPE, SIGHUP})
		sigaddset(&sigset, sig);
	pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

	// Start the signal thread
	std::thread(SignalThread).detach();
}
#endif

} // namespace Sys

// Global operator new/delete override to not throw an exception when out of
// memory. Instead, it is preferable to simply crash with an error.
void* operator new(size_t n)
{
	void* p = malloc(n);
	if (!p)
		Sys::Error("Out of memory");
	return p;
}
void operator delete(void* p) NOEXCEPT
{
	free(p);
}

// GCC expects a 16-byte aligned stack but Windows only guarantees 4-byte alignment.
// The MinGW startup code should be handling this but for some reason it isn't.
#if defined(_WIN32) && defined(__GNUC__)
#define ALIGN_STACK __attribute__((force_align_arg_pointer))
#else
#define ALIGN_STACK
#endif

// Program entry point. On Windows, main is #defined to SDL_main which is
// invoked by SDLmain.
ALIGN_STACK int main(int argc, char** argv)
{
	Sys::SetupCrashHandler();
	Sys::ParseCmdline(argc, argv);

	// Platform-specific initialization
#ifdef _WIN32
	// Don't let SDL set the timer resolution. We do that manually in Sys::Sleep.
	SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "0");

	// Mark the process as DPI-aware on Vista and above
	HMODULE user32 = LoadLibrary("user32.dll");
	if (user32) {
		FARPROC pSetProcessDPIAware = GetProcAddress(user32, "SetProcessDPIAware");
		if (pSetProcessDPIAware)
			pSetProcessDPIAware();
		FreeLibrary(user32);
	}
#else
	Sys::StartSignalThread();
#endif
	// TODO: Initialize console
	// TODO: Initialize Filesystem
	return 0;
}
