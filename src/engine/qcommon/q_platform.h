/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef Q_PLATFORM_H_
#define Q_PLATFORM_H_

// this is for determining if we have an asm version of a C function
#define idx64 0
#define idx64_32 0

#ifdef VM_COMPILED
#undef VM_COMPILED
#endif

#ifdef Q3_VM

#define id386         0
#define idppc         0
#define idppc_altivec 0
#define idsparc       0

#else

#if ( defined( _M_IX86 ) || defined( __i386__ )) && !defined( C_ONLY )
#define id386       1
#if defined SIMD_3DNOW
#define id386_3dnow 1
#else
#define id386_3dnow 0
#endif
#if defined( SIMD_SSE ) //|| 1 //|| defined(__SSE__)//defined(_MSC_VER)
#define id386_sse   1
#include <xmmintrin.h>
#define SSEVEC3_T
#else
#define id386_sse   0
#endif
#else
#define id386       0
#define id386_3dnow 0
#define id386_sse   0
#endif

#if ( defined( powerc ) || defined( powerpc ) || defined( ppc ) || \
        defined( __ppc ) || defined( __ppc__ )) && !defined( C_ONLY )
#define idppc         1
#if defined( __VEC__ )
#define idppc_altivec 1
#ifdef MACOS_X // Apple's GCC does this differently than the GNU's.
#define VECCONST_UINT8(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
        (vector unsigned char) ( a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p )
#else
#define VECCONST_UINT8(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
        (vector unsigned char) {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p }
#endif
#else
#define idppc_altivec 0
#endif
#else
#define idppc         0
#define idppc_altivec 0
#endif

#if defined( __sparc__ ) && !defined( C_ONLY )
#define idsparc 1
#else
#define idsparc 0
#endif

#endif

#ifndef __ASM_I386__ // don't include the C bits if included from qasm.h

// for windows fastcall option
#define QDECL
#define QCALL

//================================================================= WIN64/32 ===
#ifndef Q3_VM
#if defined( _WIN64 ) || defined( __WIN64__ )
#undef idx64
#define idx64 1

#define MAC_STATIC

#undef QDECL
#define QDECL       __cdecl

#undef QCALL
#define QCALL       __stdcall

#undef DLLEXPORT
#define DLLEXPORT   __declspec(dllexport)

#if defined( _MSC_VER )
#define OS_STRING   "win_msvc64"
#elif defined __MINGW64__
#define OS_STRING   "win_mingw64"
#endif

#define INLINE   __inline
#define PATH_SEP    '\\'

#if defined( __WIN64__ )
#define ARCH_STRING "x86_64"
#define VM_COMPILED
#elif defined _M_ALPHA
#define ARCH_STRING "AXP"
#endif

#define Q3_LITTLE_ENDIAN

#define DLL_PREFIX    ""
#define DLL_EXT       ".dll"

#elif defined( __WIN32__ ) || defined( _WIN32 )

#define MAC_STATIC

#undef QDECL
#define QDECL       __cdecl

#undef QCALL
#define QCALL       __stdcall

#undef DLLEXPORT
#define DLLEXPORT   __declspec(dllexport)

#if defined( _MSC_VER )
#define OS_STRING   "win_msvc"
#elif defined __MINGW32__
#define OS_STRING   "win_mingw"
#endif

#define INLINE   __inline
#define PATH_SEP    '\\'

#if defined( _M_IX86 ) || defined( __i386__ )
#define ARCH_STRING "i386"
#define VM_COMPILED
#elif defined _M_ALPHA
#define ARCH_STRING "AXP"
#endif

#define Q3_LITTLE_ENDIAN

#define DLL_PREFIX    ""
#define DLL_EXT       ".dll"

#endif

//============================================================== MAC OS X ===

#if defined( MACOS_X ) || defined( __APPLE_CC__ )

#define MAC_STATIC

// make sure this is defined, just for sanity's sake...
#ifndef MACOS_X
#define MACOS_X
#endif

#define OS_STRING   "macosx"
#define INLINE   __inline
#define PATH_SEP    '/'

#ifdef __ppc__
#define ARCH_STRING "ppc"
#define Q3_BIG_ENDIAN
#elif defined __i386__
#define ARCH_STRING "i386"
#define Q3_LITTLE_ENDIAN
#define VM_COMPILED
#elif defined __x86_64__
#undef idx64
#define idx64       1
#define ARCH_STRING "x86_64"
#define Q3_LITTLE_ENDIAN
#define VM_COMPILED
#endif

#define DLL_PREFIX    "lib"
#define DLL_EXT       ".dylib"

#endif

//================================================================= LINUX ===

#ifdef __linux__

#define MAC_STATIC

#include <endian.h>

#define OS_STRING   "linux"
#define INLINE   __inline
#define PATH_SEP    '/'

#if defined __i386__
#define ARCH_STRING "i386"
#define VM_COMPILED
#elif defined __x86_64__ && defined _ILP32
#undef idx64_32
#define idx64_32    1
#define ARCH_STRING "x32"
#define VM_COMPILED
#elif defined __x86_64__
#undef idx64
#define idx64       1
#define ARCH_STRING "x86_64"
#define VM_COMPILED
#elif defined __powerpc64__
#define ARCH_STRING "ppc64"
#elif defined __powerpc__
#define ARCH_STRING "ppc"
#elif defined __s390__
#define ARCH_STRING "s390"
#elif defined __s390x__
#define ARCH_STRING "s390x"
#elif defined __ia64__
#define ARCH_STRING "ia64"
#elif defined __alpha__
#define ARCH_STRING "alpha"
#elif defined __sparc__
#define ARCH_STRING "sparc"
#elif defined __arm__
#define ARCH_STRING "arm"
#elif defined __cris__
#define ARCH_STRING "cris"
#elif defined __hppa__
#define ARCH_STRING "hppa"
#elif defined __mips__
#define ARCH_STRING "mips"
#elif defined __sh__
#define ARCH_STRING "sh"
#endif

#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_PREFIX    "lib"
#define DLL_EXT       ".so"

#endif

//=================================================================== BSD ===

#if defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ ) || defined( __DragonFly__ )

#include <sys/types.h>
#include <machine/endian.h>

#define MAC_STATIC

#ifndef __BSD__
#define __BSD__
#endif

#if defined( __FreeBSD__ )
#define OS_STRING   "freebsd"
#elif defined( __OpenBSD__ )
#define OS_STRING   "openbsd"
#elif defined( __NetBSD__ )
#define OS_STRING   "netbsd"
#elif defined( __DragonFly__ )
#define OS_STRING   "dragonfly"
#endif

#define INLINE   __inline
#define PATH_SEP    '/'

#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __amd64__
#define ARCH_STRING "amd64"
#elif defined __axp__
#define ARCH_STRING "alpha"
#elif defined __x86_64__
#undef idx64
#define idx64       1
#define ARCH_STRING "x86_64"
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_PREFIX    "lib"
#define DLL_EXT       ".so"

#endif

//================================================================= SUNOS ===

#ifdef __sun

#include <stdint.h>
#include <sys/byteorder.h>

#define MAC_STATIC

#define OS_STRING   "solaris"
#define INLINE   __inline
#define PATH_SEP    '/'

#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __sparc
#define ARCH_STRING "sparc"
#endif

#if defined( _BIG_ENDIAN )
#define Q3_BIG_ENDIAN
#elif defined( _LITTLE_ENDIAN )
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_PREFIX    "lib"
#define DLL_EXT       ".so"

#endif

//================================================================== IRIX ===

#ifdef __sgi

#define MAC_STATIC

#define OS_STRING     "irix"
#define INLINE     __inline
#define PATH_SEP      '/'

#define ARCH_STRING   "mips"

#define Q3_BIG_ENDIAN // SGI's MIPSes are always big endian

#define DLL_PREFIX    "lib"
#define DLL_EXT       ".so"

#endif
#endif
//================================================================== Q3VM ===

#ifdef Q3_VM

#define INLINE

#define MAC_STATIC

#define OS_STRING     "q3vm"
#define PATH_SEP      '/'

#define ARCH_STRING   "bytecode"

#define DLL_PREFIX    ""
#define DLL_EXT       ".qvm"

#endif

//===========================================================================

//catch missing defines in above blocks
#if !defined( OS_STRING )
#error "Operating system not supported"
#endif

#if !defined( ARCH_STRING )
#error "Architecture not supported"
#endif

#ifndef INLINE
#error "INLINE not defined"
#endif

#ifndef PATH_SEP
#error "PATH_SEP not defined"
#endif

#ifndef DLL_EXT
#error "DLL_EXT not defined"
#endif

//endianness
short ShortSwap( short l );
int   LongSwap( int l );
float FloatSwap( float f );

#if defined( Q3_BIG_ENDIAN ) && defined( Q3_LITTLE_ENDIAN )
#error "Endianness defined as both big and little"
#elif defined( Q3_BIG_ENDIAN )

#define LittleShort(x) ShortSwap(x)
#define LittleLong(x)  LongSwap(x)
#define LittleFloat(x) FloatSwap(x)
#define BigShort
#define BigLong
#define BigFloat

#elif defined( Q3_LITTLE_ENDIAN )

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort(x) ShortSwap(x)
#define BigLong(x)  LongSwap(x)
#define BigFloat(x) FloatSwap(x)

#elif defined( Q3_VM )

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort
#define BigLong
#define BigFloat

#else
#error "Endianness not defined"
#endif

#endif

#ifndef VM_COMPILED
#define NO_VM_COMPILED
#endif

#endif /* Q_PLATFORM_H_ */
