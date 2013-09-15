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

#include "ConsoleHistory.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include <vector>
#include <memory>
#include <algorithm>

//TODO: make it thread safe.
namespace Console {

    static const char* HISTORY_FILE = "conhistory";

    static const int SAVED_HISTORY_LINES = 512;

    static std::vector<std::string> lines;

    void SaveHistory() {
        fileHandle_t f = FS_SV_FOpenFileWrite(HISTORY_FILE);

        if (!f) {
            Com_Printf(_("Couldn't write %s.\n"), HISTORY_FILE);
            return;
        }

        for (unsigned i = std::max(0UL, (lines.size()) - SAVED_HISTORY_LINES); i < lines.size(); i++) {
            FS_Write(lines[i].data(), lines[i].size(), f);
            FS_Write("\n", 1, f);
        }

        FS_FCloseFile(f);
    }

    void LoadHistory() {
        fileHandle_t f;
        int len = FS_SV_FOpenFileRead(HISTORY_FILE, &f);

        if (!f) {
            Com_Printf(_("Couldn't read %s.\n"), HISTORY_FILE);
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
