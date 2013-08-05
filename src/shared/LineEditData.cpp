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

#include "LineEditData.h"

#include <string.h>

//TODO: use a std::string instead of a fixed size char* ?
//TODO: support MAJ-selection ?

namespace Util {
    LineEditData::LineEditData(int size, int scrollSize)
    :buffer(new char[size]), bufferSize(size), scrollSize(scrollSize), width(size) {
        buffer[0] = '\0';
    }

    LineEditData::~LineEditData() {
        delete[] buffer;
    }

    const char* LineEditData::GetText() {
        return buffer;
    }

    const char* LineEditData::GetViewText() {
        return buffer + scroll;
    }

    int LineEditData::GetCursorPos() {
        return cursor;
    }

    int LineEditData::GetViewCursorPos() {
        return cursor - scroll;
    }

    void LineEditData::CursorLeft() {
        if (cursor > 0) {
            cursor --;

            if (cursor < scroll) {
                scroll = std::min(scroll - scrollSize, 0);
            }
        }
    }

    void LineEditData::CursorRight() {
        if (cursor < strlen(buffer)) {
            cursor ++;

            if (cursor > scroll + width) {
                scroll = cursor - width + scrollSize;
            }
        }
    }

    void LineEditData::CursorStart() {
        cursor = 0;
        scroll = 0;
    }

    void LineEditData::CursorEnd() {
        cursor = strlen(buffer);

        if (cursor > scroll + width) {
            scroll = cursor - width + scrollSize;
        }
    }

    void LineEditData::DeleteNext() {
        int length = strlen(buffer);

        if (cursor < length) {
            memmove(buffer + cursor, buffer + cursor + 1, length - cursor);
        }
    }

    void LineEditData::DeletePrev() {
        if (cursor > 0) {
            CursorLeft();
            DeleteNext();
        }
    }

    void LineEditData::AddChar(char a) {
        int length = strlen(buffer);

        if (length + 1 < bufferSize) {
            memmove(buffer + cursor + 1, buffer + cursor, length - cursor + 1);
            buffer[cursor] = a;

            cursor ++;
            if (cursor > scroll + width) {
                scroll = cursor - width + scrollSize;
            }
        }
    }

    void LineEditData::Clear() {
        buffer[0] = '\0';
        scroll = 0;
        cursor = 0;
    }

    void LineEditData::SetWidth(int width_) {
        width = width_;

        if (scroll <= cursor - width) {
            scroll = cursor - width + 1;
        }
    }
}
