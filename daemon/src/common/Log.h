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

#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

namespace Log {

    /*
     * There are 4 log levels that code can use:
     *   - WARNING when something is not going as expected, it is very visible.
     *   - NOTICE when we want to say something interesting, not very visible.
     *   - VERBOSE when we want to give status update, not visible by default.
     *   - DEBUG when we want to give deep info useful only to developers.
     */

    enum class Level {
        DEBUG,
        VERBOSE,
        NOTICE,
        WARNING,
    };

    // The default filtering level
    const Level DEFAULT_FILTER_LEVEL = Level::WARNING;

    /*
     * Loggers are used to group logs by subsystems and allow logs
     * to be filtered by log level by subsystem. They are used like so
     * in a submodule "Foo" in a module "bar"
     *
     *   static Logger fooLog("bar.foo", "[foo]"); // filters with the default filtering level
     *   // and adds [foo] before all messages
     *
     *   fooLog.Warn("%s %i", string, int); // "appends" the newline automatically
     *   fooLog.Debug(<expensive formatting>); // if the log is filtered, no formatting occurs
     *
     *   // However functions calls will still be performed.
     *   // To run code depending on the logger state use the following:
     *   fooLog.DoNoticeCode([&](){
     *       ExpensiveCall();
     *       fooLog.Notice("Printing the expensive expression %s", <the expression>);
     *   });
     *
     * In addition the user/developer can control the filtering level with
     *   /set logs.logLevel.foo.bar {warning, info, verbose, debug}
     */

    class Logger {
        public:
            Logger(Str::StringRef name, std::string prefix = "", Level defaultLevel = DEFAULT_FILTER_LEVEL);

            template<typename ... Args>
            void Warn(Str::StringRef format, Args&& ... args);

            template<typename ... Args>
            void Notice(Str::StringRef format, Args&& ... args);

            template<typename ... Args>
            void Verbose(Str::StringRef format, Args&& ... args);

            template<typename ... Args>
            void Debug(Str::StringRef format, Args&& ... args);

            template<typename F>
            void DoWarnCode(F&& code);

            template<typename F>
            void DoNoticeCode(F&& code);

            template<typename F>
            void DoVerboseCode(F&& code);

            template<typename F>
            void DoDebugCode(F&& code);

        private:
            std::string Prefix(std::string message);

            // the cvar logs.logLevel.<name>
            Cvar::Cvar<Level> filterLevel;

            // a prefix appended to all the messages of this logger
            std::string prefix;
    };

    /*
     * When debugging a function or before a logger is introduced for
     * a module the following functions can be used for less typing.
     * However it shouldn't stay in production code because it
     * cannot be filtered and will clutter the console.
     */

    template<typename ... Args>
    void Warn(Str::StringRef format, Args&& ... args);

    template<typename ... Args>
    void Notice(Str::StringRef format, Args&& ... args);

    template<typename ... Args>
    void Verbose(Str::StringRef format, Args&& ... args);

    template<typename ... Args>
    void Debug(Str::StringRef format, Args&& ... args);

    /*
     * A log Event, sent to the log system along a list of targets to output
     * it to. Event are not all generated by the loggers (e.g. kill messages)
     */

    struct Event {
        Event(std::string text)
            : text(std::move(text)) {}
        std::string text;
    };

    /*
     * The list of potential targets for a log event.
     * TODO: avoid people having to do (1 << TARGET1) | (1 << TARGET2) ...
     */

    enum TargetId {
        GRAPHICAL_CONSOLE,
        TTY_CONSOLE,
        CRASHLOG,
        LOGFILE,
        GAMELOG,
        HUD,
        MAX_TARGET_ID
    };

    //Internals

    // Functions used for Cvar<Log::Level>
    bool ParseCvarValue(std::string value, Log::Level& result);
    std::string SerializeCvarValue(Log::Level value);

    // Common entry points for all the formatted logs of the same level
    // (decide to which log targets the event goes)
    void CodeSourceWarn(std::string message);
    void CodeSourceNotice(std::string message);
    void CodeSourceVerbose(std::string message);
    void CodeSourceDebug(std::string message);

    // Engine calls available everywhere

    void Dispatch(Log::Event event, int targetControl);

    // Implementation of templates

    // Logger

    template<typename ... Args>
    void Logger::Warn(Str::StringRef format, Args&& ... args) {
        if (filterLevel.Get() <= Level::WARNING) {
            CodeSourceWarn(Prefix(Str::Format(format, std::forward<Args>(args) ...)));
        }
    }

    template<typename ... Args>
    void Logger::Notice(Str::StringRef format, Args&& ... args) {
        if (filterLevel.Get() <= Level::NOTICE) {
            CodeSourceNotice(Prefix(Str::Format(format, std::forward<Args>(args) ...)));
        }
    }

    template<typename ... Args>
    void Logger::Verbose(Str::StringRef format, Args&& ... args) {
        if (filterLevel.Get() <= Level::VERBOSE) {
            CodeSourceVerbose(Prefix(Str::Format(format, std::forward<Args>(args) ...)));
        }
    }

    template<typename ... Args>
    void Logger::Debug(Str::StringRef format, Args&& ... args) {
        if (filterLevel.Get() <= Level::DEBUG) {
            CodeSourceDebug(Prefix(Str::Format(format, std::forward<Args>(args) ...)));
        }
    }

    template<typename F>
    inline void Logger::DoWarnCode(F&& code) {
        if (filterLevel.Get() <= Level::WARNING) {
            code();
        }
    }

    template<typename F>
    inline void Logger::DoNoticeCode(F&& code) {
        if (filterLevel.Get() <= Level::NOTICE) {
            code();
        }
    }

    template<typename F>
    inline void Logger::DoVerboseCode(F&& code) {
        if (filterLevel.Get() <= Level::VERBOSE) {
            code();
        }
    }

    template<typename F>
    inline void Logger::DoDebugCode(F&& code) {
        if (filterLevel.Get() <= Level::DEBUG) {
            code();
        }
    }

    // Quick Logs

    template<typename ... Args>
    void Warn(Str::StringRef format, Args&& ... args) {
        CodeSourceWarn(Str::Format(format, std::forward<Args>(args) ...));
    }

    template<typename ... Args>
    void Notice(Str::StringRef format, Args&& ... args) {
        CodeSourceNotice(Str::Format(format, std::forward<Args>(args) ...));
    }

    template<typename ... Args>
    void Verbose(Str::StringRef format, Args&& ... args) {
        CodeSourceVerbose(Str::Format(format, std::forward<Args>(args) ...));
    }

    template<typename ... Args>
    void Debug(Str::StringRef format, Args&& ... args) {
        CodeSourceDebug(Str::Format(format, std::forward<Args>(args) ...));
    }
}

namespace Cvar {
    template<>
    std::string GetCvarTypeName<Log::Level>();
}

#endif //COMMON_LOG_H_
