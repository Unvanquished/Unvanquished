/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Daemon Source Code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

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
#if __GNUC__ * 100 + __GNUC_MINOR__ <= 407
#define GCC_BROKEN_CXX11
#endif

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
#define NOEXCEPT
#define NOEXCEPT_IF(x)
#define NOEXCEPT_EXPR(x) false
// Work around lack of C99 support
#define __func__ __FUNCTION__

#elif defined(Q3_VM)
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
#ifdef Q3_VM
#define STATIC_INLINE
#define QDECL
#define INLINE
#define Q_EXPORT
#ifdef Q3_VM_INSTANTIATE
#define IFDECLARE
#else
#define IFDECLARE ;
#endif
#else // Q3_VM
#define Q3_VM_INSTANTIATE
#define QDECL
#if defined(_MSC_VER) && !defined(__cplusplus)
#define INLINE __inline
#else
#define INLINE inline
#endif
#define STATIC_INLINE static INLINE
#define Q_EXPORT DLLEXPORT
#define IFDECLARE
#endif // Q3_VM

#endif // COMMON_COMPILER_H_
