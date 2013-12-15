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

#include <string>
#include <algorithm>
#include "../libs/tinyformat/tinyformat.h"

#ifndef COMMON_STRING_H_
#define COMMON_STRING_H_

namespace Str {

    template<typename T> class BasicStringRef {
    public:
        static const size_t npos = -1;

        BasicStringRef(const std::basic_string<T>& other)
        {
            ptr = other.c_str();
            len = other.size();
        }
        BasicStringRef(const T* other)
        {
            ptr = other;
            len = std::char_traits<T>::length(other);
        }

        const T& operator[](size_t pos) const
        {
            return ptr[pos];
        }
        const T* begin() const
        {
            return ptr;
        }
        const T* end() const
        {
            return ptr + len;
        }
        const T* data() const
        {
            return ptr;
        }
        const T* c_str() const
        {
            return ptr;
        }
        const T& front() const
        {
            return ptr[0];
        }
        const T& back() const
        {
            return ptr[len - 1];
        }

        std::basic_string<T> str() const
        {
            return std::basic_string<T>(ptr, len);
        }
        operator std::basic_string<T>() const
        {
            return str();
        }

        bool empty() const
        {
            return len == 0;
        }
        size_t size() const
        {
            return len;
        }

        std::basic_string<T> substr(size_t pos = 0, size_t count = npos)
        {
            if (count == npos)
                count = std::max<size_t>(len - pos, 0);
            return std::basic_string<T>(ptr + pos, count);
        }

        size_t find(BasicStringRef str, size_t pos = 0)
        {
            if (pos > len)
                return npos;
            const T* result = std::search(ptr + pos, ptr + len, str.ptr, str.ptr + str.len);
            if (result == ptr + len)
                return npos;
            return result - ptr;
        }
        size_t find(T chr, size_t pos = 0)
        {
            if (pos >= len)
                return npos;
            const T* result = std::char_traits<T>::find(ptr + pos, len - pos, chr);
            if (result == nullptr)
                return npos;
            return result - ptr;
        }
        size_t rfind(BasicStringRef str, size_t pos = npos)
        {
            pos = std::min(pos + str.len, len);
            const T* result = std::find_end(ptr, ptr + pos, str.ptr, str.ptr + str.len);
            if (str.len != 0 && result == ptr + pos)
                return npos;
            return result - ptr;
        }
        size_t rfind(T chr, size_t pos = npos)
        {
            pos = std::min(pos + 1, len);
            for (const T* p = ptr + pos; p != ptr;) {
                if (*--p == chr)
                    return p - ptr;
            }
            return npos;
        }

        int compare(BasicStringRef other) const
        {
            int result = std::char_traits<T>::compare(ptr, other.ptr, std::min(len, other.len));
            if (result != 0)
                return result;
            else
                return len - other.len;
        }
        friend bool operator==(BasicStringRef a, BasicStringRef b)
        {
            return a.compare(b) == 0;
        }
        friend bool operator!=(BasicStringRef a, BasicStringRef b)
        {
            return a.compare(b) != 0;
        }
        friend bool operator<(BasicStringRef a, BasicStringRef b)
        {
            return a.compare(b) < 0;
        }
        friend bool operator<=(BasicStringRef a, BasicStringRef b)
        {
            return a.compare(b) <= 0;
        }
        friend bool operator>(BasicStringRef a, BasicStringRef b)
        {
            return a.compare(b) > 0;
        }
        friend bool operator>=(BasicStringRef a, BasicStringRef b)
        {
            return a.compare(b) >= 0;
        }
        friend std::ostream& operator<<(std::ostream& stream, BasicStringRef str)
        {
            return stream << str.c_str();
        }
        friend std::basic_string<T> operator+(BasicStringRef a, BasicStringRef b)
        {
            std::basic_string<T> out;
            out.reserve(a.size() + b.size());
            out.append(a.data(), a.size());
            out.append(b.data(), b.size());
            return out;
        }

    private:
        const T* ptr;
        size_t len;
    };
    typedef BasicStringRef<char> StringRef;

    int ToInt(Str::StringRef text);
    bool ToInt(Str::StringRef text, int& result);

    // Locale-independent versions of ctype
    inline bool cisdigit(char c)
    {
        return c >= '0' && c <= '9';
    }
    inline bool cisupper(char c)
    {
        return c >= 'A' && c <= 'Z';
    }
    inline bool cislower(char c)
    {
        return c >= 'a' && c <= 'z';
    }
    inline bool cisalpha(char c)
    {
        return cisupper(c) || cislower(c);
    }
    inline bool cisalnum(char c)
    {
        return cisalpha(c) || cisdigit(c);
    }
    inline bool cisspace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
    inline bool cisxdigit(char c)
    {
        return cisdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }
    inline char ctolower(char c)
    {
        if (cisupper(c))
            return c - 'A' + 'a';
        else
            return c;
    }
    inline char ctoupper(char c)
    {
        if (cislower(c))
            return c - 'a' + 'A';
        else
            return c;
    }

    std::string ToLower(Str::StringRef text);

    bool IsPrefix(Str::StringRef prefix, Str::StringRef text);
    bool IsSuffix(Str::StringRef suffix, Str::StringRef text);
    int LongestPrefixSize(Str::StringRef text1, Str::StringRef text2);

    // Case insensitive versions
    bool IsIPrefix(Str::StringRef prefix, Str::StringRef text);
    int LongestIPrefixSize(Str::StringRef text1, Str::StringRef text2);

    // Case insensitive Hash and Equal functions for maps
    struct IHash {
        size_t operator()(Str::StringRef str) const
        {
            return std::hash<std::string>()(ToLower(str));
        }
    };
    struct IEqual {
        bool operator()(Str::StringRef a, Str::StringRef b) const
        {
            if (a.size() != b.size())
                return false;
            for (size_t i = 0; i < a.size(); i++) {
                if (tolower(a[i]) != tolower(b[i]))
                    return false;
            }
            return true;
        }
    };

    std::u32string UTF8To32(Str::StringRef str);
    std::string UTF32To8(Str::BasicStringRef<char32_t> str);

#ifdef _WIN32
    std::wstring UTF8To16(Str::StringRef str);
    std::string UTF16To8(Str::BasicStringRef<wchar_t> str);
#endif

    inline std::string Format(Str::StringRef format) {
        return format;
    }
    template<typename ... Args>
    std::string Format(Str::StringRef format, Args&& ... args) {
        return tinyformat::format(format.c_str(), std::forward<Args>(args) ...);
    }
}

#endif //COMMON_STRING_H_
