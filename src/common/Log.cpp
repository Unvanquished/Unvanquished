/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "Log.h"

#include "../engine/framework/LogSystem.h"
#include "../engine/qcommon/qcommon.h"

namespace Log {

    Logger::Logger(Str::StringRef name, Level defaultLevel)
    :filterLevel("logs.logLevel." + name, "Log::Level - filters out the logs from '" + name + "' below the level specified", 0, defaultLevel) {
    }

    bool ParseCvarValue(std::string value, Log::Level& result) {
        if (value == "error" or value == "err") {
            result = Log::ERROR;
            return true;
        }

        if (value == "warning" or value == "warn") {
            result = Log::WARNING;
            return true;
        }

        if (value == "info" or value == "notice") {
            result = Log::NOTICE;
            return true;
        }

        if (value == "debug" or value == "all") {
            result = Log::DEBUG;
            return true;
        }

        return false;
    }

    std::string SerializeCvarValue(Log::Level value) {
        switch(value) {
            case Log::ERROR:
                return "error";
            case Log::WARNING:
                return "warning";
            case Log::NOTICE:
                return "notice";
            case Log::DEBUG:
                return "debug";
            default:
                return "";
        }
    }

	//TODO add the time (broken for now because it is journaled) use Sys_Milliseconds instead (Utils::Milliseconds ?)
    static const int errorTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << CRASHLOG) | (1 << HUD) | (1 << LOGFILE);
    void CodeSourceError(std::string message) {
        Log::Dispatch({/*Com_Milliseconds()*/0, "^1Error: " + message}, errorTargets);
    }

    static const int warnTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << CRASHLOG) | (1 << LOGFILE);
    void CodeSourceWarn(std::string message) {
        Log::Dispatch({/*Com_Milliseconds()*/0, "^3Warn: " + message}, warnTargets);
    }

    static const int noticeTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE) | (1 << CRASHLOG) | (1 << LOGFILE);
    void CodeSourceNotice(std::string message) {
        Log::Dispatch({/*Com_Milliseconds()*/0, message}, noticeTargets);
    }

    static const int debugTargets = (1 << GRAPHICAL_CONSOLE) | (1 << TTY_CONSOLE);
    void CodeSourceDebug(std::string message) {
        Log::Dispatch({/*Com_Milliseconds()*/0, "^5Debug: " + message}, debugTargets);
    }
}

namespace Cvar {
    template<>
    std::string GetCvarTypeName<Log::Level>() {
        return "log level";
    }
}
