
#include "CrashDump.h"
#ifdef USE_BREAKPAD
#ifdef _WIN32
#include "breakpad/client/windows/handler/exception_handler.h"
#include "System.h"
#elif defined(__linux__)
#include "breakpad/client/linux/handler/exception_handler.h"
#include "breakpad/client/linux/crash_generation/crash_generation_server.h"
#endif
#endif

namespace Sys {

static std::string CrashDumpPath() {
    return FS::Path::Build(FS::GetHomePath(), "crashdump/");
}

bool CreateCrashDumpPath() {
    std::error_code createDirError;
    FS::RawPath::CreatePathTo(FS::Path::Build(CrashDumpPath(), "x"), createDirError);
    bool success = createDirError == createDirError.default_error_condition();
    if (!success) {
#ifdef _WIN32
        Log::Warn("Failed to create crash dump directory: %s", Win32StrError(GetLastError()));
#else
        Log::Warn("Failed to create crash dump directory: %s", strerror(errno));
#endif
    }
    return success;
}

// Records a crash dump sent from the VM in minidump format. This is the same
// format that Breakpad uses, but nacl minidump does not require Breakpad to work.
void NaclCrashDump(Util::RawBytes dump) {
    if(dump.size > 600000) { //it shouldn't be bigger than the buffer in nacl's code
        Log::Warn("Ignoring NaCl crash dump request: size too large");
    } else {
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count();
        std::string path = FS::Path::Build(CrashDumpPath(), Str::Format("crash-nacl-%s.dmp", std::to_string(time)));
        std::error_code err;

        //Note: the file functions always use binary mode on Windows so there shouldn't be any lossiness.
        auto file = FS::RawPath::OpenWrite(path, err);
        if (err == err.default_error_condition()) file.Write(dump.data, dump.size, err);
        if (err == err.default_error_condition()) file.Close(err);

        if (err == err.default_error_condition()) {
            Log::Notice("Wrote crash dump to %s", path);
        } else {
            Log::Warn("Error while writing crash dump");
        }
    }
    delete[] dump.data;
}

#ifdef USE_BREAKPAD

static std::string CrashServerPath() {
#ifdef _WIN32
    std::string name = "crash_server.exe";
#else
    std::string name = "crash_server";
#endif
    return FS::Path::Build(FS::GetLibPath(), name);
}

static Cvar::Cvar<bool> enableBreakpad("enableBreakpad", "If enabled on startup, starts a process for recording crash dumps", 0, true);

#ifdef _WIN32

static std::unique_ptr<google_breakpad::ExceptionHandler> crashHandler;

bool BreakpadInit() {
    if (!enableBreakpad.Get()) {
        return false;
    }

    std::string crashDir = CrashDumpPath();
    std::string executable = CrashServerPath();
    std::string pipeName = GetSingletonSocketPath() + "-crash";
    DWORD pid = GetCurrentProcessId();
    std::string cmdLine = "\"" + executable + "\" " + pipeName + " \"" + crashDir + " \" " + std::to_string(pid);

    STARTUPINFOA startInfo{};
    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags = 0;

    PROCESS_INFORMATION procInfo;
    if (!CreateProcessA(&executable[0], &cmdLine[0],
        NULL, NULL, FALSE, 0, NULL, NULL,
        &startInfo, &procInfo))
    {
        Log::Warn("Failed to start crash logging server: %s", Win32StrError(GetLastError()));
        return false;
    }

    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    std::wstring wPipeName = Str::UTF8To16(pipeName);
    crashHandler.reset(new google_breakpad::ExceptionHandler(
        Str::UTF8To16(crashDir),
        nullptr,
        nullptr,
        nullptr,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal,
        wPipeName.c_str(),
        nullptr));
    return true;
}

#elif defined(__linux__)

std::unique_ptr<google_breakpad::ExceptionHandler> crashHandler;

static bool CreateReportChannel(int& fdServer, int& fdClient) {
    if (!google_breakpad::CrashGenerationServer::CreateReportChannel(&fdServer, &fdClient)) {
        return false;
    }
    // Breakpad's function makes the client fd inheritable and the server not,
    // but we want the opposite.
    int oldflags;
    return -1 != (oldflags = fcntl(fdServer, F_GETFD))
        && -1 != fcntl(fdServer, F_SETFD, oldflags & ~FD_CLOEXEC)
        && -1 != (oldflags = fcntl(fdClient, F_GETFD))
        && -1 != fcntl(fdClient, F_SETFD, oldflags | FD_CLOEXEC);
}

bool BreakpadInit() {
    if (!enableBreakpad.Get()) {
        return false;
    }

    int fdServer, fdClient;
    if (!CreateReportChannel(fdServer, fdClient)) {
        Log::Warn("Failed to start crash logging server");
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        Log::Warn("Failed to start crash logging server: %s", strerror(errno));
        return false;
    } else if (pid != 0) { //Breakpad server MUST be in parent of engine process
        std::string fdServerStr = std::to_string(fdServer);
        std::string crashDir = CrashDumpPath();
        std::string exePath = CrashServerPath();
        std::string pidStr = std::to_string(pid);
        const char* args[] = { exePath.c_str(), fdServerStr.c_str(), crashDir.c_str(), pidStr.c_str(), nullptr };
        execv(exePath.c_str(), const_cast<char * const *>(args));
        _exit(1);
    }

    crashHandler.reset(new google_breakpad::ExceptionHandler(
        google_breakpad::MinidumpDescriptor{}, nullptr, nullptr,
        nullptr, true, fdClient));
    return true;
}

#endif //linux

#else //USE_BREAKPAD

bool BreakpadInit() {
    return false;
}

#endif //USE_BREAKPAD

} //namespace Sys
