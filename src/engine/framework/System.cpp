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
#include "../sys/sys_local.h"
#include "ConsoleHistory.h"
#include "CommandSystem.h"
#include "System.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#if defined(_WIN32) || defined(BUILD_CLIENT)
#include <SDL.h>
#endif

namespace Sys {

// Common code for fatal and non-fatal exit
static void Shutdown(bool error, Str::StringRef message)
{
	CL_Shutdown();
	SV_Shutdown(error ? va("Server fatal crashed: %s\n", message.c_str()) : va("%s\n", message.c_str()));
	Com_Shutdown();

	// Always run CON_Shutdown, because it restores the terminal to a usable state.
	CON_Shutdown();

	// Always run SDL_Quit, because it restores system resolution and gamma.
#if defined(_WIN32) || defined(BUILD_CLIENT)
	SDL_Quit();
#endif
}

void Quit(Str::StringRef message)
{
	Shutdown(false, message);

	exit(0);
}

void Error(Str::StringRef message)
{
	// Crash immediately in case of a recursive error
	static bool errorEntered = false;
	if (errorEntered)
		_exit(-1);
	errorEntered = true;

	Log::Error(message);

	Shutdown(true, message);

#if defined(_WIN32) || defined(BUILD_CLIENT)
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PRODUCT_NAME, message.c_str(), nullptr);
#endif

	exit(1);
}

// Translate non-fatal signals into a quit command
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
	// Block all signals we are interested in. This will cause the signals to be
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

#ifdef _WIN32
static HANDLE singletonSocket;
#else
static int singletonSocket;
#endif

// Get the path of a singleton socket
static std::string GetSingletonSocketPath()
{
	// Use the hash of the homepath to identify instances sharing a homepath
	const std::string& homePath = FS::GetHomePath();
	char homePathHash[33] = "";
	Com_MD5Buffer(homePath.data(), homePath.size(), homePathHash, sizeof(homePathHash));
#ifdef _WIN32
	return std::string("\\\\.\\pipe\\") + homePathHash;
#else
	// We use a temporary directory rather that using the homepath because
	// socket paths are limited to about 100 characters.
	return std::string("/tmp/." PRODUCT_NAME_LOWER "-") + homePathHash + "/socket";
#endif
}

// Try to create a socket to listen for commands from other instances
static bool CreateSingletonSocket(Str::StringRef socketPath)
{
#ifdef _WIN32
	singletonSocket = CreateNamedPipeA(socketPath.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES, 4096, 4096, 1000, NULL);
	if (!singletonSocket) {
		if (GetLastError() == ERROR_ACCESS_DENIED)
			Log::Notice("Existing instance detected");
		else
			Sys::Error("Could not create singleton socket: %s", Sys::Win32StrError(GetLastError()));
		return false;
	}
	return true;
#else
	// Make sure the socket is only accessible by the current user
	mode_t oldMode = umask(0700);
	try {
		FS::RawPath::CreatePathTo(socketPath);
	} catch (std::system_error& err) {
		Sys::Error("Could not create temporary directory for singleton socket: %s", err.what());
	}
	umask(oldMode);

	singletonSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (singletonSocket == -1)
		Sys::Error("Could not create singleton socket: %s", strerror(errno));

	if (fcntl(singletonSocket, F_SETFL, fcntl(singletonSocket, F_GETFL) | O_NONBLOCK) == -1)
		Sys::Error("Could not make socket non-blocking: %s", strerror(errno));

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	Q_strncpyz(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path));
	if (bind(singletonSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
		if (errno == EADDRINUSE)
			Log::Notice("Existing instance detected");
		else
			Sys::Error("Could not bind singleton socket: %s", strerror(errno));
		close(singletonSocket);
		return false;
	}

	if (listen(singletonSocket, SOMAXCONN) == -1)
		Sys::Error("Could not listen on singleton socket: %s", strerror(errno));

	return true;
#endif
}

// Try to connect to an existing socket to send our commands to an existing instance
static void WriteSingletonSocket(Str::StringRef socketPath, Str::StringRef commands)
{
#ifdef _WIN32
	HANDLE sock;
	while (true) {
		sock = CreateFileA(socketPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, NULL);
		if (sock != INVALID_HANDLE_VALUE)
			break;
		if (GetLastError() != ERROR_PIPE_BUSY) {
			Log::Warn("Could not create socket: %s", Sys::Win32StrError(GetLastError()));
			return;
		}
		WaitNamedPipeA(socketPath.c_str(), NMPWAIT_WAIT_FOREVER);
	}

	DWORD result = 0;
	if (!WriteFile(sock, commands.data(), commands.size(), NULL, NULL))
		Log::Warn("Could not send commands through socket: %s", Sys::Win32StrError(GetLastError()));
	else if (result != commands.size())
		Log::Warn("Could not send commands through socket: Short write");

	CloseHandle(sock);
#else
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
		Log::Warn("Could not create socket: %s", strerror(errno));
		return;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	Q_strncpyz(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path));
	if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
		Log::Warn("Could not connect to existing instance: %s", strerror(errno));
		close(sock);
		return;
	}

	ssize_t result = write(sock, commands.data(), commands.size());
	if (result == -1 || static_cast<size_t>(result) != commands.size())
		Log::Warn("Could not send commands through socket: %s", result == -1 ? strerror(errno) : "Short write");

	close(sock);
#endif
}

// Handle any commands sent by other instances
#ifdef _WIN32
static void ReadSingletonSocketCommands()
{
	// Switch the pipe to blocking mode to read the message
	DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
	SetNamedPipeHandleState(singletonSocket, &mode, NULL, NULL);

	std::string commands;
	char buffer[4096];
	while (true) {
		DWORD result = 0;
		if (!ReadFile(singletonSocket, buffer, sizeof(buffer), &result, NULL)) {
			Log::Warn("Singleton socket ReadFile() failed: %s", Sys::Win32StrError(GetLastError()));
			commands.clear();
			break;
		}
		if (result == 0)
			break;
		commands.append(buffer, result);
	}

	Cmd::BufferCommandText(commands);

	// Switch the pipe back to non-blocking mode to handle connections
	mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
	SetNamedPipeHandleState(singletonSocket, &mode, NULL, NULL);
	DisconnectNamedPipe(singletonSocket);
}
#else
static void ReadSingletonSocketCommands(int sock)
{
	std::string commands;
	char buffer[4096];
	while (true) {
		ssize_t result = read(sock, buffer, sizeof(buffer));
		if (result == -1) {
			Log::Warn("Singleton socket read() failed: %s", strerror(errno));
			return;
		}
		if (result == 0)
			break;
		commands.append(buffer, result);
	}

	Cmd::BufferCommandText(commands);
}
#endif
void ReadSingletonSocket()
{
#ifdef _WIN32
	while (true) {
		if (ConnectNamedPipe(singletonSocket, NULL))
			continue;
		switch (GetLastError()) {
		case ERROR_PIPE_LISTENING:
			// No more incoming connections
			return;
		case ERROR_PIPE_CONNECTED:
			// A client is connected
			ReadSingletonSocketCommands();
			break;
		default:
			Log::Warn("Singleton socket ConnectNamedPipe() failed: %s", Sys::Win32StrError(GetLastError()));
			return;
		}
	}
#else
	while (true) {
		int sock = accept(singletonSocket, NULL, NULL);
		if (sock == -1) {
			if (errno != EAGAIN)
				Log::Warn("Singleton socket accept() failed: %s", strerror(errno));
			return;
		}

		ReadSingletonSocketCommands(sock);
	}
#endif
}

// Command line arguments
struct cmdlineArgs_t {
	// The Windows client defaults to curses off because of performance issues
#if defined(_WIN32) && defined(BUILD_CLIENT)
	bool use_curses = false;
#else
	bool use_curses = true;
#endif

	bool reset_config = false;

	bool use_basepath = true;
	std::string homepath = FS::DefaultHomePath();
	std::vector<std::string> paths;

	std::unordered_map<std::string, std::string> cvars;
	std::vector<std::string> commands;
};

// Parse the command line arguments
#ifdef DEDICATED
#define HELP_URL ""
#else
#define HELP_URL " [" URI_SCHEME "ADDRESS[:PORT]]"
#endif
static void ParseCmdline(int argc, char** argv, cmdlineArgs_t& cmdlineArgs)
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

		// If a URI is given, transform it into a /connect command. This only
		// applies if no +commands have been given. Any arguments after the URI
		// are discarded.
		if (Str::IsIPrefix(URI_SCHEME, argv[i])) {
			cmdlineArgs.commands.push_back("connect " + Cmd::Escape(argv[i]));
			if (i != argc - 1)
				Log::Warn("Ignoring extraneous arguments after URI");
			return;
		}

		if (!strcmp(argv[i], "--help")) {
			fprintf(stderr, PRODUCT_NAME " " PRODUCT_VERSION "\n"
			                "Usage: %s" HELP_URL " [OPTION] [+COMMAND]...\n",
			                argv[0]);
			exit(0);
		} else if (!strcmp(argv[i], "--version")) {
			fprintf(stderr, PRODUCT_NAME " " PRODUCT_VERSION "\n");
			exit(0);
#ifdef __APPLE__
		} else if (!strncmp(argv[i], "-psn", 4) {
			// Ignore -psn parameters added by OSX
#endif
		} else if (!strcmp(argv[i], "-set")) {
			if (i >= argc - 2) {
				Log::Warn("Missing argument for -set");
				continue;
			}
			cmdlineArgs.cvars[argv[i + 1]] = argv[i + 2];
			i += 2;
		} else if (!strcmp(argv[i], "-nobasepath")) {
			cmdlineArgs.use_basepath = false;
		} else if (!strcmp(argv[i], "-path")) {
			if (i == argc - 1) {
				Log::Warn("Missing argument for -path");
				continue;
			}
			cmdlineArgs.paths.push_back(argv[i + 1]);
			i++;
		} else if (!strcmp(argv[i], "-homepath")) {
			if (i == argc - 1) {
				Log::Warn("Missing argument for -homepath");
				continue;
			}
			cmdlineArgs.homepath = argv[i + 1];
			cmdlineArgs.paths.push_back(argv[i + 1]);
			i++;
		} else if (!strcmp(argv[i], "-resetconfig")) {
			cmdlineArgs.reset_config = true;
		} else if (!strcmp(argv[i], "-curses")) {
			cmdlineArgs.use_curses = true;
		} else if (!strcmp(argv[i], "-nocurses")) {
			cmdlineArgs.use_curses = false;
		} else {
			Log::Warn("Ignoring unrecognized parameter \"%s\"", argv[i]);
			continue;
		}
	}
}

// Apply a -set argument early, before the configuration files are loaded
static void EarlyCvar(Str::StringRef name, cmdlineArgs_t& cmdlineArgs)
{
	auto it = cmdlineArgs.cvars.find(name);
	if (it != cmdlineArgs.cvars.end())
		Cvar::SetValue(it->first, it->second);
}

// Initialize the engine
static void Init(int argc, char** argv)
{
	cmdlineArgs_t cmdlineArgs;

	Sys::SetupCrashHandler();
	Sys::ParseCmdline(argc, argv, cmdlineArgs);

	// Platform-specific initialization
#ifdef _WIN32
	// Don't let SDL set the timer resolution. We do that manually in Sys::Sleep.
	SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "0");

	// Mark the process as DPI-aware on Vista and above, ignore any errors
	std::string errorString;
	Sys::DynamicLib user32 = Sys::DynamicLib::Open("user32.dll", errorString);
	if (user32) {
		auto pSetProcessDPIAware = user32.LoadSym<BOOL (WINAPI*)()>("SetProcessDPIAware", errorString);
		if (pSetProcessDPIAware)
			pSetProcessDPIAware();
	}
#else
	// Translate non-fatal signals to a quit command
	Sys::StartSignalThread();

	// Force a UTF-8 locale for LC_CTYPE so that terminals can output unicode
	// characters. We keep all other locale facets as "C".
	if (!setlocale(LC_CTYPE, "C.UTF-8") || !setlocale(LC_CTYPE, "en_US.UTF-8")) {
		// Try using the user's locale with UTF-8
		std::string locale = setlocale(LC_CTYPE, NULL);
		size_t dot = locale.rfind('.');
		if (dot != std::string::npos)
			locale.replace(dot, locale.size() - dot, ".UTF-8");
		if (!setlocale(LC_CTYPE, locale.c_str()))
			Log::Warn("Could not set locale to UTF-8, unicode characters may not display correctly");
	}
#endif

	// Initialize the console
	if (cmdlineArgs.use_curses)
		CON_Init();
	else
		CON_Init_TTY();

	// Initialize the filesystem
	if (cmdlineArgs.use_basepath)
		cmdlineArgs.paths.insert(cmdlineArgs.paths.begin(), FS::DefaultBasePath());
	FS::Initialize(cmdlineArgs.homepath, cmdlineArgs.paths);

	// Look for an existing instance of the engine running on the same homepath.
	// If there is one, forward any +commands to it and exit.
	std::string socketPath = GetSingletonSocketPath();
	if (!CreateSingletonSocket(socketPath)) {
		if (!cmdlineArgs.commands.empty()) {
			Log::Notice("Forwarding commands to existing instance");
			WriteSingletonSocket(socketPath, std::accumulate(cmdlineArgs.commands.begin(), cmdlineArgs.commands.end(), std::string()));
		} else
			Log::Notice("No commands given, exiting...");
		exit(0);
	}

	// Load the base paks
	// TODO: cvar names and FS_* stuff needs to be properly integrated
	EarlyCvar("fs_basepak", cmdlineArgs);
	EarlyCvar("fs_extrapaks", cmdlineArgs);
	FS_LoadBasePak();

	// Load configuration files
#ifndef BUILD_SERVER
	Cmd::BufferCommandText("preset default.cfg");
#endif
	if (!cmdlineArgs.reset_config) {
#ifdef BUILD_SERVER
		Cmd::BufferCommandText("exec -f " SERVERCONFIG_NAME);
#else
		Cmd::BufferCommandText("exec -f " CONFIG_NAME);
		Cmd::BufferCommandText("exec -f " KEYBINDINGS_NAME);
		Cmd::BufferCommandText("exec -f " AUTOEXEC_NAME);
#endif
	}
	Cmd::ExecuteCommandBuffer();

	// Override any cvars set in configuration files with those on the command-line
	for (auto& cvar: cmdlineArgs.cvars)
		Cvar::SetValue(cvar.first, cvar.second);

	// Load the console history
	Console::LoadHistory();
}

} // namespace Sys

// GCC expects a 16-byte aligned stack but Windows only guarantees 4-byte alignment.
// The MinGW startup code should be handling this but for some reason it isn't.
#if defined(_WIN32) && defined(__GNUC__) && defined(__i386__)
#define ALIGN_STACK __attribute__((force_align_arg_pointer))
#else
#define ALIGN_STACK
#endif

// Program entry point. On Windows, main is #defined to SDL_main which is
// invoked by SDLmain.
ALIGN_STACK int main(int argc, char** argv)
{
	Sys::Init(argc, argv);
	return 0;
}
