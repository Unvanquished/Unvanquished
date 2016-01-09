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

// Abstract away compiler-specific stuff

#ifndef COMMON_COMPILER_H_
#define COMMON_COMPILER_H_

// GCC and Clang
#ifdef __GNUC__

// Emit a nice warning when a function is used
#define DEPRECATED __attribute__((__deprecated__))

// A cold function is rarely called, so branches that lead to one are assumed
// to be unlikely
#define COLD __attribute__((__cold__))

// Indicates that a function does not return
#define NORETURN __attribute__((__noreturn__))
#define NORETURN_PTR __attribute__((__noreturn__))

// Expect printf-style arguments for a function: a is the index of the format
// string, and b is the index of the first variable argument
#define PRINTF_LIKE(n) __attribute__((__format__(__printf__, n, (n) + 1)))
#define VPRINTF_LIKE(n) __attribute__((__format__(__printf__, n, 0)))
#define PRINTF_TRANSLATE_ARG(a) __attribute__((__format_arg__(a)))

// Marks that this function will return a pointer that is not aliased to any
// other pointer
#define MALLOC_LIKE __attribute__((__malloc__))

// Align the address of a variable to a certain value
#define ALIGNED(a, x) x __attribute__((__aligned__(a)))

// Shared library function import/export
#ifdef _WIN32
#define DLLEXPORT __attribute__((__dllexport__))
#define DLLIMPORT __attribute__((__dllimport__))
#else
#define DLLEXPORT __attribute__((__visibility__("default")))
#define DLLIMPORT __attribute__((__visibility__("default")))
#endif

// override and final keywords
#if __clang__ || (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
#define OVERRIDE override
#define FINAL final
#else
#define OVERRIDE
#define FINAL
#endif

// noexcept keyword, this should be used on all move constructors and move
// assignments so that containers move objects instead of copying them.
#define NOEXCEPT noexcept
#define NOEXCEPT_IF(x) noexcept(x)
#define NOEXCEPT_EXPR(x) noexcept(x)

// Work around lack of constexpr
#define CONSTEXPR constexpr

// To mark functions which cause issues with address sanitizer
#if __clang__
#if __has_attribute(no_sanitize_address)
# define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif
#elif __SANITIZE_ADDRESS__
# define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

// GCC 4.6 has incomplete support for C++11
#if !defined(__clang__) && __GNUC__ * 100 + __GNUC_MINOR__ <= 407
#define GCC_BROKEN_CXX11
#endif
#ifdef __cplusplus
#include <new>
#if defined(__GLIBCXX__) && (__GLIBCXX__ == 20110325 || __GLIBCXX__ == 20110627 || __GLIBCXX__ == 20111026 || __GLIBCXX__ == 20120301 || __GLIBCXX__ == 20130412)
#define LIBSTDCXX_BROKEN_CXX11
#endif
#endif

// Support for explicitly defaulted functions
#define HAS_EXPLICIT_DEFAULT

// Microsoft Visual C++
#elif defined( _MSC_VER )

// Disable some warnings
#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4032)
#pragma warning(disable : 4051)
#pragma warning(disable : 4057) // slightly different base types
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4115)
#pragma warning(disable : 4125) // decimal digit terminates octal escape sequence
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4136)
#pragma warning(disable : 4152) // nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#pragma warning(disable : 4244)
//#pragma warning(disable : 4142)   // benign redefinition
#pragma warning(disable : 4305) // truncation from const double to float
//#pragma warning(disable : 4310)   // cast truncates constant value
//#pragma warning(disable : 4505)   // unreferenced local function has been removed
#pragma warning(disable : 4514)
#pragma warning(disable : 4702) // unreachable code
#pragma warning(disable : 4711) // selected for automatic inline expansion
#pragma warning(disable : 4220) // varargs matches remaining parameters
#pragma warning(disable : 4706) // assignment within conditional expression // cs: probably should correct all of these at some point
#pragma warning(disable : 4005) // macro redefinition
#pragma warning(disable : 4996) // This function or variable may be unsafe. Consider using 'function_s' instead
#pragma warning(disable : 4075) // initializers put in unrecognized initialization area
#pragma warning(disable : 4355) // 'this': used in member initializer list
#pragma warning(disable : 4305) // signed unsigned mismatch
#pragma warning(disable : 4554) // qualifier applied to reference type; ignored
#pragma warning(disable : 4800) // forcing bool variable to one or zero, possible performance loss
#pragma warning(disable : 4090) // 'function' : different 'const' qualifiers
#pragma warning(disable : 4267) // 'initializing' : conversion from 'size_t' to 'int', possible loss of data
#pragma warning(disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable : 4133) // 'function' : incompatible types - from 'unsigned long *' to 'const time_t *'
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4389) // '==' : signed/unsigned mismatch

// See descriptions above
#define DEPRECATED __declspec(deprecated)
#define COLD
#define NORETURN __declspec(noreturn)
#define NORETURN_PTR
#define PRINTF_LIKE(n)
#define VPRINTF_LIKE(n)
#define PRINTF_TRANSLATE_ARG(a)
#define MALLOC_LIKE __declspec(restrict)
#define ALIGNED(a,x) __declspec(align(a)) x
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#define OVERRIDE override
#define FINAL final
// VS2015 supports this
#if _MSC_VER >= 1900
#define NOEXCEPT noexcept
#define NOEXCEPT_IF(x) noexcept(x)
#define NOEXCEPT_EXPR(x) false
#define HAS_EXPLICIT_DEFAULT
#else
#define NOEXCEPT
#define NOEXCEPT_IF(x)
#define NOEXCEPT_EXPR(x) false
#endif
// Work around lack of C99 support
#define __func__ __FUNCTION__
// Work around lack of constexpr
#define CONSTEXPR const
#define ATTRIBUTE_NO_SANITIZE_ADDRESS

// Other compilers, unsupported
#else
#warning "Unsupported compiler"
#define DEPRECATED
#define COLD
#define NORETURN
#define PRINTF_LIKE(n)
#define VPRINTF_LIKE(n)
#define PRINTF_TRANSLATE_ARG(a)
#define MALLOC_LIKE
#define ALIGNED(a,x) x
#define DLLEXPORT
#define DLLIMPORT
#endif

// Compat macros
#define QDECL
#if defined(_MSC_VER) && !defined(__cplusplus)
#define INLINE __inline
#else
#define INLINE inline
#endif
#define Q_EXPORT DLLEXPORT

// Uses SD-6 Feature Test Recommendations
#ifdef __cpp_constexpr
#   if __cpp_constexpr >= 201304
#       define CONSTEXPR_FUNCTION_RELAXED constexpr
#   else
#       define CONSTEXPR_FUNCTION_RELAXED
#   endif
#   define CONSTEXPR_FUNCTION constexpr
#else
// Work around lack of constexpr
#   define CONSTEXPR_FUNCTION
#   define CONSTEXPR_FUNCTION_RELAXED
#endif

#endif // COMMON_COMPILER_H_
