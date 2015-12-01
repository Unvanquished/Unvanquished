#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>

#include "client/linux/crash_generation/crash_generation_server.h"

google_breakpad::CrashGenerationServer* server;

void ShutdownServer(int) {
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

    struct sigaction sa{};
    sa.sa_handler = ShutdownServer;
    if (sigaction(SIGTERM, &sa, nullptr) != 0) return 1;

    int fd = std::stoi(argv[1]);
    std::string path = argv[2];
    pid_t pid = std::stoi(argv[3]);

    server = new google_breakpad::CrashGenerationServer(fd, nullptr, nullptr, nullptr, nullptr, true, &path);
    if (!server->Start()) {
        return 1;
    }

    int _;
    waitpid(pid, &_, 0); //wait for the game to exit
    ShutdownServer(0);
    return 0;
}
