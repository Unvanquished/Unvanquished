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

#include <unistd.h>
#include <signal.h>
#include <string>

#include "breakpad/src/client/linux/crash_generation/crash_generation_server.h"

google_breakpad::CrashGenerationServer* server;

void ShutdownServer() {
    // Destructor waits for crash dump to finish
    delete server;
}

/*
    Starts a Breakpad crash generation server to enable out-of-process crash dumps.
    First argument: file descriptor to listen on
    Second argument: crash dump directory
    Third argument: PID of engine (child process)
*/
int main(int argc, char** argv) {

    if (argc != 4) {
        return 1;
    }

    int fd = std::stoi(argv[1]);
    std::string path = argv[2];
    pid_t pid = std::stoi(argv[3]);

    if (atexit(ShutdownServer) != 0) return 1;
    server = new google_breakpad::CrashGenerationServer(fd, nullptr,
            nullptr, nullptr, nullptr, true, &path);
    if (!server->Start()) {
        return 1;
    }

    sigset_t set;
    if (0 != sigemptyset(&set) ||
        0 != sigaddset(&set, SIGCHLD) ||
        0 != sigprocmask(SIG_BLOCK, &set, nullptr)) {
        return 1;
    }
    int _;
    sigwait(&set, &_);
    return 0;
}
