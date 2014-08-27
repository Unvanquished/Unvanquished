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
#include "CommandSystem.h"
#include "System.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
#if defined(_WIN32) || defined(BUILD_CLIENT)
#include <SDL.h>
#endif

namespace Sys {

// Common code for fatal and non-fatal exit
static void Shutdown(Str::StringRef message)
{
	CL_Shutdown();
	SV_Shutdown(message.empty() ? "Server quit" : va("Server fatal crashed: %s\n", message));
	Com_Shutdown();

	// Always run SDL_Quit, because it restore system resolution and gamma.
#if defined(_WIN32) || defined(BUILD_CLIENT)
	SDL_Quit();
#endif

	// TODO: shutdown console
}

void Quit()
{
	Shutdown("");

	exit(0);
}

void Error(Str::StringRef message)
{
	static bool errorEntered = false;
	if (errorEntered)
		_exit(-1);
	errorEntered = true;

	Log::Error(message);

	Shutdown(message);

#if defined(_WIN32) || defined(BUILD_CLIENT)
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Unvanquished", message.c_str(), nullptr);
#endif

	exit(1);
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
	Sys::SetupCrashHandler();
	Sys::ParseCmdline(argc, argv);

	// Platform-specific initialization
#ifdef _WIN32
	// Don't let SDL set the timer resolution. We do that manually in Sys::Sleep.
	SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "0");

	// Mark the process as DPI-aware on Vista and above, ignore any errors
	{
		std::string errorString;
		Sys::DynamicLib user32 = Sys::DynamicLib::Open("user32.dll", errorString);
		if (user32) {
			FARPROC pSetProcessDPIAware = user32.LoadSym<FARPROC>("SetProcessDPIAware", errorString);
			if (pSetProcessDPIAware)
				pSetProcessDPIAware();
		}
	}
#else
	Sys::StartSignalThread();
#endif
	// TODO: Initialize console
	// TODO: Initialize Filesystem
	return 0;
}
