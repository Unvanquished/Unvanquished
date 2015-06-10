/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "ConsoleHistory.h"

//TODO: make it thread safe.
namespace Console {
#ifndef BUILD_SERVER
    static const char* HISTORY_FILE = "conhistory";
#else
    static const char* HISTORY_FILE = "conhistory_server";
#endif
    static const int SAVED_HISTORY_LINES = 512;

    static std::vector<std::string> lines;

    void SaveHistory() {
        fileHandle_t f = FS_SV_FOpenFileWrite(HISTORY_FILE);

        if (!f) {
            Com_Printf("Couldn't write %s.\n", HISTORY_FILE);
            return;
        }

        for (unsigned i = std::max(0L, ((long)lines.size()) - SAVED_HISTORY_LINES); i < lines.size(); i++) {
            FS_Write(lines[i].data(), lines[i].size(), f);
            FS_Write("\n", 1, f);
        }

        FS_FCloseFile(f);
    }

    void LoadHistory() {
        fileHandle_t f;
        int len = FS_SV_FOpenFileRead(HISTORY_FILE, &f);

        if (!f) {
            Com_Printf("Couldn't read %s.\n", HISTORY_FILE);
            return;
        }

        std::unique_ptr<char[]> buffer(new char[len + 1]);

        FS_Read(buffer.get(), len, f);
        buffer[len] = '\0';
        FS_FCloseFile(f);

        char* buf = buffer.get();
        char* end;
        while ((end = strchr(buf, '\n'))) {
            *end = '\0';
            lines.push_back(buf);
            buf = end + 1;
        }
    }

    static const std::string& GetLine(HistoryHandle handle)
    {
        static std::string empty = "";
        if (handle == HISTORY_END) {
            return empty;
        } else {
            return lines[handle];
        }
    }

    void AddToHistory(HistoryHandle& handle, std::string current) {
        if (lines.empty() || current != lines.back()) {
            lines.push_back(std::move(current));
        }
        handle = HISTORY_END;

        //TODO defer it more? when the programs exits?
        SaveHistory();
    }

    void PrevLine(HistoryHandle& handle, std::string& current) {
        if (!current.empty() && current != GetLine(handle)) {
            lines.push_back(current);
        }

        if (handle == 0 || (handle == HISTORY_END && lines.empty())) {
            return;
        } else if (handle == HISTORY_END) {
            handle = lines.size() -1;
        } else {
            handle --;
        }
        current = GetLine(handle);
    }

    void NextLine(HistoryHandle& handle, std::string& current) {
        if (!current.empty() && current != GetLine(handle)) {
            lines.push_back(current);
        }

        if (handle == HISTORY_END) {
            current.clear();
        } else if (handle == lines.size() - 1) {
            handle = HISTORY_END;
            current.clear();
        } else {
            handle ++;
            current = GetLine(handle);
        }
    }

}
