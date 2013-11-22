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
#include <algorithm>

//TODO: use a std::string instead of a fixed size char* ?
//TODO: support MAJ-selection ?

namespace Util {
    LineEditData::LineEditData(int size, int scrollSize)
    :scrollSize(scrollSize), width(size), scroll(0), cursor(0) {
    }
    const std::u32string& LineEditData::GetText() const {
        return buffer;
    }
    std::u32string& LineEditData::GetText() {
        return buffer;
    }

    const char32_t* LineEditData::GetViewText() const {
        return GetText().c_str() + scroll;
    }

    unsigned LineEditData::GetViewStartPos() const {
        return scroll;
    }

    unsigned LineEditData::GetCursorPos() const {
        return cursor;
    }

    unsigned LineEditData::GetViewCursorPos() const {
        return cursor - scroll;
    }

    void LineEditData::SetText(std::u32string text) {
        buffer = std::move(text);
        cursor = buffer.size();
        if (cursor > width) {
            scroll = cursor - width + scrollSize;
        } else
            scroll = 0;
    }

    void LineEditData::CursorLeft(int times) {
        cursor = std::max(0, (int)cursor - times);
        UpdateScroll();
    }

    void LineEditData::CursorRight(int times) {
        cursor = std::min(cursor + times, (unsigned)buffer.size());
        UpdateScroll();
    }

    void LineEditData::CursorStart() {
        cursor = 0;
        scroll = 0;
    }

    void LineEditData::CursorEnd() {
        cursor = buffer.size();
        UpdateScroll();
    }

    void LineEditData::SetCursor(int pos) {
        cursor = pos;
        UpdateScroll();
    }

    void LineEditData::DeleteNext(int times) {
        buffer.erase(cursor, times);
    }

    void LineEditData::DeletePrev(int times) {
        times = std::min(times, (int)cursor);

        CursorLeft(times);
        DeleteNext(times);
    }

    void LineEditData::AddChar(char32_t a) {
        buffer.insert(buffer.begin() + cursor, a);
        cursor ++;
        UpdateScroll();
    }

    void LineEditData::Clear() {
        buffer.clear();
        scroll = 0;
        cursor = 0;
    }

    void LineEditData::SetWidth(int width_) {
        width = width_;
        UpdateScroll();
    }

    unsigned LineEditData::GetWidth() const {
        return width;
    }

    void LineEditData::UpdateScroll() {
        if (cursor < scroll) {
            scroll = std::min(scroll - scrollSize, 0U);
        } else if (cursor > scroll + width) {
            scroll = cursor - width + scrollSize;
        }
    }
}
