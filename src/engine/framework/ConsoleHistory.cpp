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

//TODO: make it thread safe.
//TODO: use unicode
namespace Console {

    static const char* HISTORY_FILE = "con_history";

    static const int SAVED_HISTORY_LINES = 512;

    static std::vector<std::string> lines;

    void SaveHistory() {
        fileHandle_t f = FS_SV_FOpenFileWrite(HISTORY_FILE);

        if (!f) {
            Com_Printf(_("Couldn't write %s.\n"), HISTORY_FILE);
            return;
        }

        for (int i = std::max(0ul, lines.size() - SAVED_HISTORY_LINES); i < lines.size(); i++) {
            FS_Write(lines[i].c_str(), lines[i].size(), f);
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

        char* buffer = new char[len + 1];

        FS_Read(buffer, len, f);
        buffer[len] = '\0';
        FS_FCloseFile(f);

        char* buf = buffer;
        char* end;
        while ((end = strchr(buf, '\n'))) {
            *end = '\0';
            AddToHistory(buf);
            buf = end + 1;
        }

        AddToHistory(buf);

        delete[] buffer;
    }

    void AddToHistory(std::string line) {
        lines.push_back(std::move(line));
    }

    HistoryHandle HistoryEnd() {
        return lines.size();
    }

    std::string PrevLine(HistoryHandle& handle) {
        handle --;

        if (handle < 0) {
            handle ++;
            return "";
        }

        return lines[handle];
    }

    std::string NextLine(HistoryHandle& handle) {
        handle ++;

        if (handle >= lines.size()) {
            handle --;
            return "";
        }

        return lines[handle];
    }

}
