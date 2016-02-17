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

#ifndef COMMON_STRING_H_
#define COMMON_STRING_H_

#include <algorithm>
#include "Compiler.h"

namespace Str {
    void AssertOnTinyFormatError(std::string reason);
}

#define TINYFORMAT_ERROR(reason) Str::AssertOnTinyFormatError(#reason);
#include "tinyformat/tinyformat.h"

namespace Str {

    template<typename T> class BasicStringRef {
    public:
        static const size_t npos = -1;

        BasicStringRef(const std::basic_string<T>& other)
            : ptr(other.c_str()), len(other.size()) {}
        BasicStringRef(const T* other)
            : ptr(other), len(std::char_traits<T>::length(other)) {}

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

        std::basic_string<T> substr(size_t pos = 0, size_t count = npos) const
        {
            if (count == npos)
                count = std::max<size_t>(len - pos, 0);
            return std::basic_string<T>(ptr + pos, count);
        }

        size_t find(BasicStringRef str, size_t pos = 0) const
        {
            if (pos > len)
                return npos;
            const T* result = std::search(ptr + pos, ptr + len, str.ptr, str.ptr + str.len);
            if (result == ptr + len)
                return npos;
            return result - ptr;
        }
        size_t find(T chr, size_t pos = 0) const
        {
            if (pos >= len)
                return npos;
            const T* result = std::char_traits<T>::find(ptr + pos, len - pos, chr);
            if (result == nullptr)
                return npos;
            return result - ptr;
        }
        size_t rfind(BasicStringRef str, size_t pos = npos) const
        {
            pos = std::min(pos + str.len, len);
            const T* result = std::find_end(ptr, ptr + pos, str.ptr, str.ptr + str.len);
            if (str.len != 0 && result == ptr + pos)
                return npos;
            return result - ptr;
        }
        size_t rfind(T chr, size_t pos = npos) const
        {
            pos = (pos == npos) ? len : std::min(pos + 1, len);
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
    using StringRef = BasicStringRef<char>;

    bool ParseInt(int& value, Str::StringRef text);

    float ToFloat(Str::StringRef text);
    bool ToFloat(Str::StringRef text, float& result);

    // Locale-independent versions of ctype
    inline bool cisdigit(int c)
    {
        return c >= '0' && c <= '9';
    }
    inline bool cisupper(int c)
    {
        return c >= 'A' && c <= 'Z';
    }
    inline bool cislower(int c)
    {
        return c >= 'a' && c <= 'z';
    }
    inline bool cisalpha(int c)
    {
        return cisupper(c) || cislower(c);
    }
    inline bool cisalnum(int c)
    {
        return cisalpha(c) || cisdigit(c);
    }
    inline bool cisspace(int c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
    inline bool cisxdigit(int c)
    {
        return cisdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }
    inline int ctolower(int c)
    {
        if (cisupper(c))
            return c - 'A' + 'a';
        else
            return c;
    }
    inline int ctoupper(int c)
    {
        if (cislower(c))
            return c - 'a' + 'A';
        else
            return c;
    }

    /*
     * Converts a hexadecimal character to the value of the digit it represents.
     * Pre: cisxdigit(ch)
     */
    inline CONSTEXPR_FUNCTION int GetHex(char ch)
    {
        return ch > '9' ?
            ( ch >= 'a' ? ch - 'a' + 10 : ch - 'A' + 10 )
            : ch - '0'
        ;
    }

    std::string ToUpper(Str::StringRef text);
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
                if (ctolower(a[i]) != ctolower(b[i]))
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

namespace std {
    template <typename T>
    struct hash<Str::BasicStringRef<T>> {
        size_t operator()(const Str::BasicStringRef<T>& s) const {
            return hash<string>()(s.str()); //FIXME: avoid a copy?
        }
    };
}

#endif //COMMON_STRING_H_
