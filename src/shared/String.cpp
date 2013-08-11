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

namespace Str {

    int ToInt(const std::string& text) {
        return atoi(text.c_str());
    }

    bool ToInt(const std::string& text, int& result) {
        char* end;
        const char* start = text.c_str();
        result = strtol(start, &end, 10);
        if (errno == ERANGE || result < std::numeric_limits<int>::min() || std::numeric_limits<int>::max() < result)
            return false;
        if (start == end)
            return false;
        return true;
    }

    bool IsPrefix(const std::string& prefix, const std::string& text) {
        auto res = std::mismatch(prefix.begin(), prefix.end(), text.begin());
        if (res.first == prefix.end()) {
            return true;
        }
        return false;
    }

    int LongestPrefixSize(const std::string& text1, const std::string& text2) {
        auto res = std::mismatch(text1.begin(), text1.end(), text2.begin());

        return res.first - text1.begin();
    }

    // Unicode encoder/decoder based on http://utfcpp.sourceforge.net/
    static const uint32_t UNICODE_CODE_POINT_MAX = 0x0010ffff;
    static const uint32_t UNICODE_SURROGATE_MIN = 0xd800;
    static const uint32_t UNICODE_SURROGATE_MAX = 0xdfff;
    static const uint32_t UNICODE_REPLACEMENT_CHAR = 0xfffd;
    static std::initializer_list<char> UNICODE_REPLACEMENT_CHAR_UTF8 = {char(0xef), char(0xbf), char(0xbd)};

    static int UTF8_SequenceLength(uint8_t lead)
    {
        if (lead < 0x80)
            return 1;
        else if ((lead >> 5) == 0x6)
            return 2;
        else if ((lead >> 4) == 0xe)
            return 3;
        else if ((lead >> 3) == 0x1e)
            return 4;
        else
            return 0;
    }

    static uint32_t UTF8_DecodeSequence(const char*& it, const char* end, int length)
    {
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

        return out;
    }

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

    std::u32string UTF8To32(const std::string& str)
    {
        std::u32string out;
        const char* it = str.data();
        const char* end = it + str.size();

        while (it != end) {
            uint8_t c = *it;
            int length = UTF8_SequenceLength(c);
            uint32_t cp = UNICODE_REPLACEMENT_CHAR;
            if (length != 0)
                cp = UTF8_DecodeSequence(it, end, length);
            if (cp > UNICODE_CODE_POINT_MAX || UTF8_IsOverlongSequence(cp, length))
                cp = UNICODE_REPLACEMENT_CHAR;
            out.push_back(cp);
        }
        return out;
    }

    std::string UTF32To8(const std::u32string& str)
    {
        std::string out;
        for (uint32_t ch: str) {
            if (ch > UNICODE_CODE_POINT_MAX)
                out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
            else if (ch >= UNICODE_SURROGATE_MIN && ch <= UNICODE_SURROGATE_MAX)
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
        return out;
    }
}
