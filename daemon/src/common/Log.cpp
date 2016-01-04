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

#include "Common.h"

namespace Log {

    Logger::Logger(Str::StringRef name, Level defaultLevel)
    :filterLevel("logs.logLevel." + name, "Log::Level - logs from '" + name + "' below the level specified are filtered", 0, defaultLevel) {
    }

    bool ParseCvarValue(std::string value, Log::Level& result) {
        if (value == "warning" or value == "warn") {
            result = Log::LOG_WARNING;
            return true;
        }

        if (value == "info" or value == "notice") {
            result = Log::LOG_NOTICE;
            return true;
        }

        if (value == "verbose") {
            result = Log::LOG_VERBOSE;
            return true;
        }

        if (value == "debug" or value == "all") {
            result = Log::LOG_DEBUG;
            return true;
        }

        return false;
    }

    std::string SerializeCvarValue(Log::Level value) {
        switch(value) {
            case Log::LOG_WARNING:
                return "warning";
            case Log::LOG_NOTICE:
                return "notice";
            case Log::LOG_VERBOSE:
                return "verbose";
            case Log::LOG_DEBUG:
                return "debug";
            default:
                return "";
        }
    }

	//TODO add the time (broken for now because it is journaled) use Sys_Milliseconds instead (Utils::Milliseconds ?)
    static const int warnTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << CRASHLOG) | (1 << LOGFILE);
    void CodeSourceWarn(std::string message) {
        Log::Dispatch({"^3Warn: " + message}, warnTargets);
    }

    static const int noticeTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << CRASHLOG) | (1 << LOGFILE);
    void CodeSourceNotice(std::string message) {
        Log::Dispatch({message}, noticeTargets);
    }

    static const int verboseTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << CRASHLOG) | (1 << LOGFILE);
    void CodeSourceVerbose(std::string message) {
        Log::Dispatch({message}, verboseTargets);
    }

    static const int debugTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE);
    void CodeSourceDebug(std::string message) {
        Log::Dispatch({"^5Debug: " + message}, debugTargets);
    }
}

namespace Cvar {
    template<>
    std::string GetCvarTypeName<Log::Level>() {
        return "log level";
    }
}
