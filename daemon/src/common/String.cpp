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

namespace Str {

    void AssertOnTinyFormatError(std::string reason) {
        if (reason != "") {
            ASSERT_EQ(reason, "");
        } else {
            ASSERT_NQ(reason, "");
        }
    }

    bool ParseInt(int& value, Str::StringRef text) {
        if (text.empty())
            return false;

        // Parse the sign
        const char* p = text.begin();
        bool neg = *p == '-';
        if (*p == '-' || *p == '+') {
            if (++p == text.end())
                return false;
        }

        // Parse the digits
        value = 0;
        do {
            if (!cisdigit(*p))
                return false;

            // Check for overflow when multiplying
            int min = std::numeric_limits<int>::min();
            int max = std::numeric_limits<int>::max();
            if (neg ? value < (min + 1) / 10 : value > max / 10)
                return false;
            value *= 10;

            // Check for overflow when adding
            if (neg ? value < min + *p - '0' : value > max - *p + '0')
                return false;
            if (neg)
                value -= *p - '0';
            else
                value += *p - '0';
        } while (++p != text.end());

        return true;
    }

    float ToFloat(Str::StringRef text) {
        return atof(text.c_str());
    }

    bool ToFloat(Str::StringRef text, float& result) {
        char* end;
        const char* start = text.c_str();
        result = strtof(start, &end);
        if (errno == ERANGE)
            return false;
        if (start == end)
            return false;
        return true;
    }

    std::string ToUpper(Str::StringRef text) {
        std::string res;
        res.reserve(text.size());

        std::transform(text.begin(), text.end(), std::back_inserter(res), ctoupper);

        return res;
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
    static const uint32_t UNICODE_SURROGATE_TAIL_END = 0xdfff;
    static const uint32_t UNICODE_REPLACEMENT_CHAR = 0xfffd;
#ifdef _WIN32
    static const uint32_t UNICODE_SURROGATE_HEAD_END = 0xdbff;
    static const uint32_t UNICODE_SURROGATE_TAIL_START = 0xdc00;
    static const uint32_t UNICODE_SURROGATE_MASK = 0x3ff;
#endif
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
