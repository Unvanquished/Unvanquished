#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <string>

#include "client/linux/crash_generation/crash_generation_server.h"

using namespace google_breakpad;

CrashGenerationServer* server;

void ShutdownServer(int) {
    // Destructor waits for crash dump to finish
    delete server;
}


/*
    Starts a Breakpad crash generation server to enable out-of-process crash dumps.
    First argument: file descriptor to listen on
    Second argument: crash dump directory
*/
int main(int argc, char** argv) {

    if (argc != 2) {
        return 1;
    }

    struct sigaction sa{};
    sa.sa_handler = ShutdownServer;
    if (sigaction(SIGTERM, &sa, nullptr) != 0) return 1;
    // Receive a signal to quit when engine process dies
    if (prctl(PR_SET_PDEATHSIG, SIGTERM) != 0) return 1;

    int fd = std::stoi(argv[0]);
    std::string path = argv[1];
    server = new CrashGenerationServer(fd, nullptr, nullptr, nullptr, nullptr, true, &path);
    if (!server->Start()) {
        return 1;
    }

    pause();
    return 0;
}
