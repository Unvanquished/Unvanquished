/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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

#ifndef COMMON_PLATFORM_H_
#define COMMON_PLATFORM_H_

// Platform-specific configuration
#if defined(_WIN32)
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
#error "Platform not supported"
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

// VM Prefixes
#if !defined(VM_NAME)
#define VM_STRING_PREFIX ""
#else
#define VM_STRING_PREFIX XSTRING(VM_NAME) "."
#endif

#endif // COMMON_PLATFORM_H_
