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

#include "String.h"
#include "../engine/qcommon/q_shared.h"
#include <limits>
#include <algorithm>
#include <string.h>

namespace Str {

    int ToInt(Str::StringRef text) {
        return atoi(text.c_str());
    }

    bool ToInt(Str::StringRef text, int& result) {
        char* end;
        const char* start = text.c_str();
        result = strtol(start, &end, 10);
        if (errno == ERANGE || result < std::numeric_limits<int>::min() || std::numeric_limits<int>::max() < result)
            return false;
        if (start == end)
            return false;
        return true;
    }

    std::string ToLower(Str::StringRef text) {
        std::string res;
        res.reserve(text.size());

        std::transform(text.begin(), text.end(), std::back_inserter(res), ctolower);

        return res;
    }

    bool IsPrefix(Str::StringRef prefix, Str::StringRef text) {
        auto res = std::mismatch(prefix.begin(), prefix.end(), text.begin());
        return res.first == prefix.end();
    }

    bool IsSuffix(Str::StringRef suffix, Str::StringRef text) {
        if (text.size() < suffix.size())
            return false;
        return std::equal(text.begin() + text.size() - suffix.size(), text.end(), suffix.begin());
    }

    int LongestPrefixSize(Str::StringRef text1, Str::StringRef text2) {
        auto res = std::mismatch(text1.begin(), text1.end(), text2.begin());

        return res.first - text1.begin();
    }

    bool IsIPrefix(Str::StringRef prefix, Str::StringRef text) {
        return IsPrefix(ToLower(prefix), ToLower(text));
    }

    int LongestIPrefixSize(Str::StringRef text1, Str::StringRef text2) {
        return LongestPrefixSize(ToLower(text1), ToLower(text2));
    }

    // Unicode encoder/decoder based on http://utfcpp.sourceforge.net/
    static const uint32_t UNICODE_CODE_POINT_MAX = 0x10ffff;
    static const uint32_t UNICODE_SURROGATE_HEAD_START = 0xd800;
    static const uint32_t UNICODE_SURROGATE_HEAD_END = 0xdbff;
    static const uint32_t UNICODE_SURROGATE_TAIL_START = 0xdc00;
    static const uint32_t UNICODE_SURROGATE_TAIL_END = 0xdfff;
    static const uint32_t UNICODE_SURROGATE_MASK = 0x3ff;
    static const uint32_t UNICODE_REPLACEMENT_CHAR = 0xfffd;
    static std::initializer_list<char> UNICODE_REPLACEMENT_CHAR_UTF8 = {char(0xef), char(0xbf), char(0xbd)};

    static bool UTF8_IsOverlongSequence(uint32_t cp, int length)
    {
        if (cp < 0x80)
            return length != 1;
        else if (cp < 0x800)
            return length != 2;
        else if (cp < 0x10000)
            return length != 3;
        return false;
    }

    static uint32_t UTF8_DecodeSequence(const char*& it, const char* end)
    {
        int length;
        uint8_t head = *it;
        if (head < 0x80)
            length = 1;
        else if ((head >> 5) == 0x6)
            length = 2;
        else if ((head >> 4) == 0xe)
            length = 3;
        else if ((head >> 3) == 0x1e)
            length = 4;
        else
            return UNICODE_REPLACEMENT_CHAR;

        uint32_t out = uint8_t(*it++);

        if (length == 1)
            out &= 0x7f;
        else if (length == 2)
            out &= 0x1f;
        else if (length == 3)
            out &= 0x0f;
        else
            out &= 0x07;

        if (length >= 2) {
            if (it == end)
                return UNICODE_REPLACEMENT_CHAR;
            if (uint8_t(*it) >> 6 != 0x2)
                return UNICODE_REPLACEMENT_CHAR;
            out <<= 6;
            out |= *it++ & 0x3f;
        }
        if (length >= 3) {
            if (it == end)
                return UNICODE_REPLACEMENT_CHAR;
            if (uint8_t(*it) >> 6 != 0x2)
                return UNICODE_REPLACEMENT_CHAR;
            out <<= 6;
            out |= *it++ & 0x3f;
        }
        if (length >= 4) {
            if (it == end)
                return UNICODE_REPLACEMENT_CHAR;
            if (uint8_t(*it) >> 6 != 0x2)
                return UNICODE_REPLACEMENT_CHAR;
            out <<= 6;
            out |= *it++ & 0x3f;
        }

        if (out > UNICODE_CODE_POINT_MAX || UTF8_IsOverlongSequence(out, length))
            return UNICODE_REPLACEMENT_CHAR;
        else
            return out;
    }

    static void UTF8_Append(std::string& out, uint32_t ch)
    {
        if (ch > UNICODE_CODE_POINT_MAX)
            out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
        else if (ch >= UNICODE_SURROGATE_HEAD_START && ch <= UNICODE_SURROGATE_TAIL_END)
            out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
        else if (ch < 0x80)
            out.push_back(ch);
        else if (ch < 0x800)
            out.append({char((ch >> 6) | 0xc0), char((ch & 0x3f) | 0x80)});
        else if (ch < 0x10000)
            out.append({char((ch >> 12) | 0xe0), char(((ch >> 6) & 0x3f) | 0x80), char((ch & 0x3f) | 0x80)});
        else
            out.append({char((ch >> 18) | 0xf0), char(((ch >> 12) & 0x3f) | 0x80), char(((ch >> 6) & 0x3f) | 0x80), char((ch & 0x3f) | 0x80)});
    }

    std::u32string UTF8To32(Str::StringRef str)
    {
        std::u32string out;
        const char* it = str.data();
        const char* end = it + str.size();

        while (it != end)
            out.push_back(UTF8_DecodeSequence(it, end));
        return out;
    }

    std::string UTF32To8(Str::BasicStringRef<char32_t> str)
    {
        std::string out;
        for (uint32_t ch: str)
            UTF8_Append(out, ch);
        return out;
    }

#ifdef _WIN32
    std::wstring UTF8To16(Str::StringRef str)
    {
        std::wstring out;
        const char* it = str.data();
        const char* end = it + str.size();

        while (it != end) {
            uint32_t cp = UTF8_DecodeSequence(it, end);
            if (cp >= 0x10000)
                out.append({wchar_t(UNICODE_SURROGATE_HEAD_START | (cp >> 10)), wchar_t(UNICODE_SURROGATE_TAIL_START | (cp & UNICODE_SURROGATE_MASK))});
            else
                out.push_back(cp);
        }
        return out;
    }

    std::string UTF16To8(Str::BasicStringRef<wchar_t> str)
    {
        std::string out;
        auto it = str.begin();
        auto end = str.end();

        while (it != end) {
            uint16_t ch = *it++;
            if (ch >= UNICODE_SURROGATE_HEAD_START && ch <= UNICODE_SURROGATE_HEAD_END) {
                if (it == end)
                    out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
                else {
                    uint16_t tail = *it++;
                    if (tail >= UNICODE_SURROGATE_TAIL_START && tail <= UNICODE_SURROGATE_TAIL_END)
                        UTF8_Append(out, ((ch & UNICODE_SURROGATE_MASK) << 10) | (tail & UNICODE_SURROGATE_MASK));
                    else
                        out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
                }
            } else if (ch >= UNICODE_SURROGATE_TAIL_START && ch <= UNICODE_SURROGATE_TAIL_END)
                out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
            else
                UTF8_Append(out, ch);
        }
        return out;
    }
#endif
}
