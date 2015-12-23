#ifndef FRAMEWORK_CRASHDUMP_H_
#define FRAMEWORK_CRASHDUMP_H_

#include "common/Util.h"

namespace Sys {
    //Ensure existence of the subdirectory of homepath used for crash dumps. Return true if successful
    bool CreateCrashDumpPath();

    //Launch the Breakpad server, if enabled in CMake and cvars. Return true if it is created
    bool BreakpadInit();

    //Write a crash dump that came from an NaCl VM.
    void NaclCrashDump(Util::RawBytes dump);
}

#endif // FRAMEWORK_CRASHDUMP_H_
