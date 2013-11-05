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

#include <string>

#include "Cvar.h"
#include "String.h"

#ifndef SHARED_LOG_H_
#define SHARED_LOG_H_

namespace Log {

    struct Event {
        int timestamp;
        std::string text;
    };

    enum TargetId {
        GRAPHICAL_CONSOLE,
        TTY_CONSOLE,
        CRASHLOG,
        LOGFILE,
        GAMELOG,
        HUD,
        MAX_TARGET_ID
    };

    typedef bool TargetControl[MAX_TARGET_ID];

    enum Level {
        DEBUG,
        NOTICE,
        WARNING,
        ERROR
    };

    class Logger {
        public:
            Logger(const std::string& name, Level level);

            template<typename ... Args>
            void Error(const std::string& format, Args ... args);

            template<typename ... Args>
            void Warn(const std::string& format, Args ... args);

            template<typename ... Args>
            void Notice(const std::string& format, Args ... args);

            template<typename ... Args>
            void Debug(const std::string& format, Args ... args);

        private:
            Cvar::Cvar<Level> filterLevel;
    };

    template<typename ... Args>
    void Error(const std::string& foramt, Args ... args);

    template<typename ... Args>
    void Warn(const std::string& foramt, Args ... args);

    template<typename ... Args>
    void Notice(const std::string& foramt, Args ... args);

    template<typename ... Args>
    void Debug(const std::string& foramt, Args ... args);

    //Internals

    bool ParseCvarValue(std::string value, Log::Level& result);
    std::string SerializeCvarValue(Log::Level value);

    void CodeSourceError(std::string message);
    void CodeSourceWarn(std::string message);
    void CodeSourceNotice(std::string message);
    void CodeSourceDebug(std::string message);

    //TODO: allow commands to be run in an environnement to change the log targets (command typed in the client console -> feedbakc in the client console only, same for TTY, same for RCON)

    // Implementation of templates

    // Logger

    template<typename ... Args>
    void Logger::Error(const std::string& format, Args ... args) {
        CodeSourceError(Str::Format(format, args ...));
    }

    template<typename ... Args>
    void Logger::Warn(const std::string& format, Args ... args) {
        if (filterLevel.Get() <= WARNING) {
            CodeSourceWarn(Str::Format(format, args ...));
        }
    }

    template<typename ... Args>
    void Logger::Notice(const std::string& format, Args ... args) {
        if (filterLevel.Get() <= NOTICE) {
            CodeSourceNotice(Str::Format(format, args ...));
        }
    }

    template<typename ... Args>
    void Logger::Debug(const std::string& format, Args ... args) {
        if (filterLevel.Get() <= DEBUG) {
            CodeSourceDebug(Str::Format(format, args ...));
        }
    }

    // Quick logs

    template<typename ... Args>
    void Error(const std::string& format, Args ... args) {
        CodeSourceError(Str::Format(format, args ...));
    }

    template<typename ... Args>
    void Warn(const std::string& format, Args ... args) {
        CodeSourceWarn(Str::Format(format, args ...));
    }

    template<typename ... Args>
    void Notice(const std::string& format, Args ... args) {
        CodeSourceNotice(Str::Format(format, args ...));
    }

    template<typename ... Args>
    void Debug(const std::string& format, Args ... args) {
        CodeSourceDebug(Str::Format(format, args ...));
    }
}

#endif //SHARED_LOG_H_
