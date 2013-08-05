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

#include "ConsoleField.h"
#include "CommandSystem.h"
#include "../qcommon/q_shared.h"

//TODO: use unicode

namespace Console {

    Field::Field(): LineEditData(512) {
    }

    Field::~Field() {
        if (oldBuffer) {
            delete[] oldBuffer;
        }
    }

    void Field::HistoryPrev() {
        if (not inHistory) {
            hist = HistoryEnd();
        }

        std::string line = PrevLine(hist);

        if (line.empty()) {
            return;
        }

        if (not inHistory) {
            inHistory = true;
            oldScroll = scroll;
            oldCursor = cursor;
            oldBuffer = buffer;
            buffer = new char[bufferSize];
        }

        Q_strncpyz(buffer, line.c_str(), bufferSize);
        CursorEnd();
    }

    void Field::HistoryNext() {
        if (not inHistory) {
            return;
        }

        std::string line = NextLine(hist);
        if (not line.empty()) {
            Q_strncpyz(buffer, line.c_str(), bufferSize);
            CursorEnd();
        } else {
            delete[] buffer;
            buffer = oldBuffer;
            oldBuffer = nullptr;

            inHistory = false;
            scroll = oldScroll;
            cursor = oldCursor;
        }
    }

    void Field::RunCommand() {
        //TODO: handle com_consoleCommand
        //TODO: default command if no \ or /
        if(strlen(buffer) == 0) {
            return;
        }

        if (buffer[0] == '/' or buffer[0] == '\\') {
            AddToHistory(buffer + 1);
            Cmd::BufferCommandText(buffer + 1, Cmd::END, true);
        } else {
            AddToHistory(buffer);
            Cmd::BufferCommandText(buffer, Cmd::END, true);
        }

        inHistory = false;
        Clear();
    }

    void Field::AutoComplete() {
        //TODO
    }

}
