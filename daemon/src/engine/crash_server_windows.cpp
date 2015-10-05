
#include <string>
#include <windows.h>
#include "breakpad/client/windows/crash_generation/crash_generation_server.h"


/*
    Starts a Breakpad crash generation server to enable out-of-process crash dumps.
    First argument: pipe name
    Second argument: crash dump directory
    Third argument: engine process ID
*/
int main(int argc, char** argv)
{
    if (argc != 3)
    {
        return 1;
    }

    DWORD pid;
    try {
        pid = std::stoul(argv[2]);
    } catch (...) {
        return 1;
    }

    HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!parent) {
        return 1;
    }

    std::wstring pipeName, dumpPath;
    //convert args from UTF-8 to UTF-16...
    int len0 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, argv[0], -1, NULL, 0); //includes null char
    int len1 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, argv[1], -1, NULL, 0);
    if (!len0 || !len1) return 1;
    pipeName.resize(len0 - 1);
    dumpPath.resize(len1 - 1);
    if (MultiByteToWideChar(CP_UTF8, 0, argv[0], strlen(argv[0]), &pipeName[0], len0 - 1) != len0 - 1) return 1;
    if (MultiByteToWideChar(CP_UTF8, 0, argv[0], strlen(argv[1]), &dumpPath[0], len1 - 1) != len1 - 1) return 1;

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

    WaitForSingleObject(parent, INFINITE); //wait for application to terminate

    delete server; //destructor waits for any dump requests to finish

    return 0;
}
