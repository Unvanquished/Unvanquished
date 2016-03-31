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

#if defined(_WIN32)

    #include "Windows.h"

    namespace Sys {
        bool IsDebuggerAttached() {
            return IsDebuggerPresent() == TRUE;
        }
    }

#elif defined(__linux__)

    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/prctl.h>
    #include <sys/ptrace.h>
    #include <sys/wait.h>

    namespace Sys {
        bool IsDebuggerAttached() {
            // Remove the ptrace restriction that can be set on this process (Ubuntu restricts ptrace
            // by default).
            prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);

            // Spawn a child process that will try to ptrace our current process
            int pid = fork();

            if (pid == -1) {
                return false;
            }

            if (pid == 0) {
                int parentPid = getppid();

                // Try to trace the parent (only one process can a given process so if we are being
                // debugged, the call will fail).
                if (ptrace(PTRACE_ATTACH, parentPid, nullptr, nullptr) == 0) {
                    // No debugger is present so we started tracing. Stop tracing and return false.
                    waitpid(parentPid, nullptr, 0);
                    ptrace(PTRACE_CONT, nullptr, nullptr);
                    ptrace(PTRACE_DETACH, parentPid, nullptr, nullptr);
                    exit(0);
                } else {
                    // Call failed, we are being traced, return true.
                    exit(1);
                }

            } else {
                // Get the result from the child
                int status;
                waitpid(pid, &status, 0);
                return WEXITSTATUS(status);
            }
        }
    }

#elif defined(__APPLE__)

    // From Apple's Technical Q&A QA1361 (https://developer.apple.com/library/mac/qa/qa1361/_index.html)
    // We simply ask the kernel if we are being debugged.
    #ifndef __APPLE_API_UNSTABLE
        #define __APPLE_API_UNSTABLE
    #endif
    #include <string.h>
    #include <sys/sysctl.h>
    #include <unistd.h>

    namespace Sys {
        bool IsDebuggerAttached() {
            int mib[4] = {
                CTL_KERN,
                KERN_PROC,
                KERN_PROC_PID,
                getpid()
            };

            kinfo_proc info;
            memset(&info, 0, sizeof(info));
            size_t size = sizeof(info);

            sysctl(mib, sizeof(mib) / sizeof(mib[0]), &info, &size, nullptr, 0);

            return (info.kp_proc.p_flag & P_TRACED) != 0;
        }
    }

#elif defined(__native_client__)

    // For now, assume there is no debugger attached for NaCl VMs at very few developers use that.
    // TODO: when starting the VM in VMMain get data from the engine saying if we are in debug mode or not.
    namespace Sys {
        bool IsDebuggerAttached() {
            return false;
        }
    }

#else
#error "Debugger.cpp unimplemented for this platform"
#endif
