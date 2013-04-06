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

// Declarations common to engine & tools

#ifndef _UNV_GLOBAL_H_
#define _UNV_GLOBAL_H_

// Function attribute macros

#if defined __GNUC__ || defined __clang__
#define NORETURN __attribute__((__noreturn__))
#define UNUSED __attribute__((__unused__))
#define PRINTF_ARGS(f, a) __attribute__((__format__(__printf__, (f), (a))))
#define PRINTF_LIKE(n) PRINTF_ARGS((n), (n) + 1)
#define VPRINTF_LIKE(n) PRINTF_ARGS((n), 0)
#define ALIGNED(a,x) x __attribute__((__aligned__(a)))
#define ALWAYS_INLINE INLINE __attribute__((__always_inline__))
#elif defined( _MSC_VER )
#define NORETURN
#define UNUSED
#define PRINTF_ARGS(f, a)
#define PRINTF_LIKE(n)
#define VPRINTF_LIKE(n)
#define ALIGNED(a,x) __declspec(align(a)) x
#define ALWAYS_INLINE __forceinline
#define __attribute__(x)
#define __func__ __FUNCTION__
#else
#define NORETURN
#define UNUSED
#define PRINTF_ARGS(f, a)
#define PRINTF_LIKE(n)
#define VPRINTF_LIKE(n)
#define ALIGNED(a,x) x
#define ALWAYS_INLINE
#define __attribute__(x)
#define __func__
#endif

#endif
