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

#ifndef SHARED_PLATFORM_H_
#define SHARED_PLATFORM_H_

// SSE support
#if _M_IX86_FP >= 1 || defined(__SSE__)
#include <xmmintrin.h>
#define id386_sse 1
#endif
#if _M_IX86_FP >= 2 || defined(__SSE2__)
#include <emmintrin.h>
#define id386_sse2 1
#endif

// Platform-specific configuration
#ifdef _WIN32
#define DLL_PREFIX ""
#define DLL_EXT ".dll"
#define EXE_EXT ".exe"
#define PATH_SEP '\\'
#ifdef _WIN64
#define PLATFORM_STRING "win64"
#else
#define PLATFORM_STRING "win32"
#endif
#elif defined(__APPLE__)
#define DLL_PREFIX "lib"
#define DLL_EXT ".dylib"
#define EXE_EXT ""
#define PATH_SEP '/'
#ifdef __x86_64__
#define PLATFORM_STRING "mac64"
#elif __i386__
#define PLATFORM_STRING "mac32"
#endif
#elif defined(__linux__)
#define DLL_PREFIX "lib"
#define DLL_EXT ".so"
#define EXE_EXT ""
#define PATH_SEP '/'
#ifdef __x86_64__
#define PLATFORM_STRING "linux64"
#elif __i386__
#define PLATFORM_STRING "linux32"
#endif
#elif defined(__native_client__)
// No platform definitions required
#else
#error "Platform not supported"
#endif

#endif // SHARED_PLATFORM_H_
