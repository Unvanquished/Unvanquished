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

#include "LogSystem.h"
#include "../../common/String.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include <mutex>

namespace Log {

    static Target* targets[MAX_TARGET_ID];


    //TODO make me reentrant // or check it is actually reentrant when using for (Event e : events) do stuff
    //TODO think way more about thread safety
    void Dispatch(Log::Event event, int targetControl) {
        static std::vector<Log::Event> buffers[MAX_TARGET_ID];
        static std::recursive_mutex bufferLocks[MAX_TARGET_ID];

        for (int i = 0; i < MAX_TARGET_ID; i++) {
            if ((targetControl >> i) & 1) {
                auto& buffer = buffers[i];
                std::lock_guard<std::recursive_mutex> guard(bufferLocks[i]);

                buffer.push_back(event);

                bool processed = false;
                if (targets[i]) {
                    processed = targets[i]->Process(buffer);
                }

                if (processed) {
                    buffer.clear();
                } else if (buffer.size() > 512) {
                    buffer.clear();
                }
            }
        }
    }

    void RegisterTarget(TargetId id, Target* target) {
        targets[id] = target;
    }

    Target::Target() {
    }

    void Target::Register(TargetId id) {
        Log::RegisterTarget(id, this);
    }

    //Log Targets
    //TODO: move them in their respective modules
    //TODO this one isn't mutlithreaded at all, need a rewrite of the consoles
    class TTYTarget : public Target {
        public:
            TTYTarget() {
                this->Register(TTY_CONSOLE);
            }

            virtual bool Process(std::vector<Log::Event>& events) OVERRIDE {
                for (Log::Event event : events)  {
                    Sys_Print(event.text.c_str());
                    Sys_Print("\n");
                }
                return true;
            }
    };

    static TTYTarget tty;

    //TODO for now these change nothing because the file is opened before the cvars are read.
    //TODO add a Callback on these that will make the logFile open a new file or something
    //Or maybe have Com_Init start it ?
    Cvar::Cvar<bool> useLogFile("logs.logFile.active", "are the logs sent in the logfile", Cvar::ARCHIVE, true);
    Cvar::Cvar<std::string> logFileName("logs.logFile.filename", "the name of the logfile", Cvar::ARCHIVE, "daemon.log");
    Cvar::Cvar<bool> overwrite("logs.logFile.overwrite", "if true the logfile is deleted at each run else the logs are just appended", Cvar::ARCHIVE, true);
    Cvar::Cvar<bool> forceFlush("logs.logFile.forceFlush", "are all the logs flushed immediately (more accurate but slower)", Cvar::ARCHIVE, false);
    class LogFileTarget :public Target {
        public:
            LogFileTarget() : logFile(0), recursing(false) {
                this->Register(LOGFILE);
            }

            virtual bool Process(std::vector<Log::Event>& events) OVERRIDE {
                //If we have no log file drop the events
                if (not useLogFile.Get()) {
                    return true;
                }

                //TODO this is actually wrong because the FS itself doesn't support multiple threads
                // so we pray it works (it should because the memory touched by FS_Write seems very cold)
                std::lock_guard<std::recursive_mutex> guard(lock);

                //TODO atomic test and set on recursing
                if (logFile == 0 and FS::IsInitialized() and not recursing) {
                    recursing = true;

                    if (overwrite.Get()) {
                        logFile = FS_FOpenFileWrite(logFileName.Get().c_str());
                    } else {
                        logFile = FS_FOpenFileAppend(logFileName.Get().c_str());
                    }

                    if (forceFlush.Get()) {
                        FS_ForceFlush(logFile);
                    }

                    recursing = false;
                }

                if (logFile != 0) {
                    for (Log::Event event : events) {
                        std::string text = event.text + "\n";
                        FS_Write(text.c_str(), text.length(), logFile);
                    }
                    return true;
                } else {
                    return false;
                }
            }

        private:
            fileHandle_t logFile;
            //TODO atomic boolean
            bool recursing;
            std::recursive_mutex lock;
    };

    static LogFileTarget logfile;

    //Temp targets that forward to the regular Com_Printf.
    class LegacyTarget : public Target {
        public:
            LegacyTarget(std::string name, TargetId id): name(std::move(name)) {
                this->Register(id);
            }

            virtual bool Process(std::vector<Log::Event>& events) OVERRIDE {
                Q_UNUSED(events);

                return true;
                //Com_Printf("%s: %s\n", name.c_str(), event.text.c_str());
            }

        private:
            std::string name;
    };

    static LegacyTarget console4("GameLog", GAMELOG);
    static LegacyTarget console5("HUD", HUD);
}
