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

#ifndef COMMON_PLATFORM_H_
#define COMMON_PLATFORM_H_

// Platform-specific configuration
#ifdef _WIN32
#define DLL_EXT ".dll"
#define EXE_EXT ".exe"
#define PATH_SEP '\\'
#define PLATFORM_STRING "Windows"
#elif defined(__APPLE__)
#define DLL_EXT ".so"
#define EXE_EXT ""
#define PATH_SEP '/'
#define PLATFORM_STRING "Mac OS X"
#elif defined(__linux__)
#define DLL_EXT ".so"
#define EXE_EXT ""
#define PATH_SEP '/'
#define PLATFORM_STRING "Linux"
#elif defined(__native_client__)
#define PLATFORM_STRING "Native Client"
#else
#ifndef Q3_VM
#error "Platform not supported"
#endif
#endif

// Architecture-specific configuration
#if defined(__i386__) || defined(_M_IX86)
#undef __i386__
#define __i386__ 1
#define ARCH_STRING "x86"
#elif defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64__) || defined(_M_X64)
#undef __x86_64__
#define __x86_64__ 1
#define ARCH_STRING "x86_64"
#elif defined(__pnacl__)
#define ARCH_STRING "PNaCl"
#elif defined(Q3_VM)
#define ARCH_STRING "QVM_bytecode"
#else
#error "Architecture not supported"
#endif

// SSE support
#if defined(__x86_64__) || defined(__SSE__) || _M_IX86_FP >= 1
#include <xmmintrin.h>
#if defined(__x86_64__) || defined(__SSE2__) || _M_IX86_FP >= 2
#include <emmintrin.h>
#define idx86_sse 2
#else
#define idx86_sse 1
#endif
#endif

#endif // COMMON_PLATFORM_H_
