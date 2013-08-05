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

//TODO: make it thread safe.
//TODO: use unicode
namespace Console {

    static const char* HISTORY_FILE = "con_history";

    static const int HISTORY_LINES = 512;

    static std::string lines[HISTORY_LINES];
    static int index = -1;

    void SaveHistory() {
        fileHandle_t f = FS_SV_FOpenFileWrite(HISTORY_FILE);

        if (!f) {
            Com_Printf(_("Couldn't write %s.\n"), HISTORY_FILE);
            return;
        }

        for (int i = std::max(0, index - HISTORY_LINES + 1); i <= index; i++) {
            const std::string& line = lines[i % HISTORY_LINES];
            FS_Write(line.c_str(), line.size(), f);
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

        index = 0;
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
        index ++;
        lines[index % HISTORY_LINES] = std::move(line);
    }

    HistoryHandle HistoryEnd() {
        return index + 1;
    }

    std::string PrevLine(HistoryHandle& handle) {
        handle --;

        if (handle <= index - HISTORY_LINES) {
            handle = index - HISTORY_LINES;
        }

        if (handle <= std::max(index - HISTORY_LINES, -1)) {
            handle ++;
            return "";
        }

        return lines[handle % HISTORY_LINES];
    }

    std::string NextLine(HistoryHandle& handle) {
        handle ++;

        if (handle <= index - HISTORY_LINES) {
            handle = index - HISTORY_LINES;
        }

        if (handle > index) {
            handle --;
            return "";
        }

        return lines[handle % HISTORY_LINES];
    }

}
