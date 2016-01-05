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

#include "Common.h"

//TODO: use a std::string instead of a fixed size char* ?
//TODO: support MAJ-selection ?

namespace Util {
    LineEditData::LineEditData(unsigned size, unsigned scrollSize)
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
            scroll = cursor - width + std::min(width - 1, scrollSize);
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

    void LineEditData::SwapWithNext() {
        unsigned length = buffer.length();

        if (length > 1 && cursor) {
            if (cursor == length) {
                --cursor;
            }
            std::swap(buffer[cursor - 1], buffer[cursor]);
            ++cursor;
        }
    }

    void LineEditData::Clear() {
        buffer.clear();
        scroll = 0;
        cursor = 0;
    }

    void LineEditData::SetWidth(int width_) {
        width = std::max(width_, 1);
        UpdateScroll();
    }

    unsigned LineEditData::GetWidth() const {
        return width;
    }

    void LineEditData::UpdateScroll() {
        if (cursor < scroll) {
            scroll = scroll > scrollSize ? scroll - scrollSize : 0U;
        } else if (cursor > scroll + width) {
            scroll = cursor - width + std::min(width - 1, scrollSize);
        }
    }
}
