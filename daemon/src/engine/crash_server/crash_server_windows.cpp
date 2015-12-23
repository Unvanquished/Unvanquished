/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2015, Daemon Developers
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


#include <string>
#include <windows.h>
#include "breakpad/src/client/windows/crash_generation/crash_generation_server.h"


/*
    Starts a Breakpad crash generation server to enable out-of-process crash dumps.
    First argument: pipe name
    Second argument: crash dump directory
    Third argument: engine process ID
*/
int main(int argc, char** argv)
{
    if (argc != 4)
    {
        return 1;
    }

    DWORD pid;
    try {
        pid = std::stoul(argv[3]);
    } catch (...) {
        return 1;
    }

    HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!parent) {
        return 1;
    }

    std::wstring pipeName, dumpPath;
    // convert args from UTF-8 to UTF-16...
    int len0 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, argv[1], -1, NULL, 0); // length includes null char
    int len1 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, argv[2], -1, NULL, 0);
    if (!len0 || !len1) return 1;
    pipeName.resize(len0 - 1);
    dumpPath.resize(len1 - 1);
    if (MultiByteToWideChar(CP_UTF8, 0, argv[1], strlen(argv[1]), &pipeName[0], len0 - 1) != len0 - 1) return 1;
    if (MultiByteToWideChar(CP_UTF8, 0, argv[1], strlen(argv[2]), &dumpPath[0], len1 - 1) != len1 - 1) return 1;

    google_breakpad::CrashGenerationServer* server = new google_breakpad::CrashGenerationServer(
        pipeName,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        true,
        &dumpPath);

    if (!server->Start()) {
        return 1;
    }

    WaitForSingleObject(parent, INFINITE); // wait for application to terminate

    delete server; // destructor waits for any dump requests to finish

    return 0;
}
