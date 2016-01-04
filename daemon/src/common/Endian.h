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

// Endian detection from boost/detail/endian.hpp

#ifndef COMMON_ENDIAN_H_
#define COMMON_ENDIAN_H_

//
// Special cases come first:
//
#if defined (__GLIBC__)
// GNU libc offers the helpful header <endian.h> which defines
// __BYTE_ORDER
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define Q3_LITTLE_ENDIAN
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define Q3_BIG_ENDIAN
# elif (__BYTE_ORDER == __PDP_ENDIAN)
#  define BOOST_PDP_ENDIAN
# else
#  error Unknown machine endianness detected.
# endif
# define Q3_BYTE_ORDER __BYTE_ORDER

#elif defined(__NetBSD__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || (__DragonFly__)
//
// BSD has endian.h, see https://svn.boost.org/trac/boost/ticket/6013
#  if defined(__OpenBSD__)
#  include <machine/endian.h>
#  else
#  include <sys/endian.h>
#  endif
# if (_BYTE_ORDER == _LITTLE_ENDIAN)
#  define Q3_LITTLE_ENDIAN
# elif (_BYTE_ORDER == _BIG_ENDIAN)
#  define Q3_BIG_ENDIAN
# elif (_BYTE_ORDER == _PDP_ENDIAN)
#  define BOOST_PDP_ENDIAN
# else
#  error Unknown machine endianness detected.
# endif
# define Q3_BYTE_ORDER _BYTE_ORDER

#elif defined( __ANDROID__ )
// Android specific code, see: https://svn.boost.org/trac/boost/ticket/7528
// Here we can use machine/_types.h, see:
// http://stackoverflow.com/questions/6212951/endianness-of-android-ndk
# include "machine/_types.h"
# ifdef __ARMEB__
#  define Q3_BIG_ENDIAN
#  define Q3_BYTE_ORDER 4321
# else
#  define Q3_LITTLE_ENDIAN
#  define Q3_BYTE_ORDER 1234
# endif // __ARMEB__

#elif defined( _XBOX )
//
// XBox is always big endian??
//
# define Q3_BIG_ENDIAN
# define Q3_BYTE_ORDER 4321

#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN) || \
    defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__) || \
    defined(__BIGENDIAN__) && !defined(__LITTLEENDIAN__) || \
    defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN)
# define Q3_BIG_ENDIAN
# define Q3_BYTE_ORDER 4321
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN) || \
    defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__) || \
    defined(__LITTLEENDIAN__) && !defined(__BIGENDIAN__) || \
    defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN)
# define Q3_LITTLE_ENDIAN
# define Q3_BYTE_ORDER 1234
#elif defined(__sparc) || defined(__sparc__) \
   || defined(_POWER) || defined(__powerpc__) \
   || defined(__ppc__) || defined(__hpux) || defined(__hppa) \
   || defined(_MIPSEB) || defined(_POWER) \
   || defined(__s390__) || defined(__ARMEB__)
# define Q3_BIG_ENDIAN
# define Q3_BYTE_ORDER 4321
#elif defined(__i386__) || defined(__alpha__) \
   || defined(__ia64) || defined(__ia64__) \
   || defined(_M_IX86) || defined(_M_IA64) \
   || defined(_M_ALPHA) || defined(__amd64) \
   || defined(__amd64__) || defined(_M_AMD64) \
   || defined(__x86_64) || defined(__x86_64__) \
   || defined(_M_X64) || defined(__bfin__) \
   || defined(__ARMEL__) \
   || (defined(_WIN32) && defined(__ARM__) && defined(_MSC_VER)) // ARM Windows CE don't define anything reasonably unique, but there are no big-endian Windows versions 

# define Q3_LITTLE_ENDIAN
# define Q3_BYTE_ORDER 1234
#else
# error The file boost/detail/endian.hpp needs to be set up for your CPU type.
#endif

// Compilers are smart enough to optimize these to a single bswap instruction
inline int16_t Swap16(int16_t x)
{
	return (x >> 8) | (x << 8);
}

inline int32_t Swap32(int32_t x)
{
	return (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | (x >> 24);
}

inline int64_t Swap64(int64_t x)
{
	return  (x << 56) |
	       ((x << 40) & 0xff000000000000ULL) |
	       ((x << 24) & 0xff0000000000ULL) |
	       ((x <<  8) & 0xff00000000ULL) |
	       ((x >>  8) & 0xff000000ULL) |
	       ((x >> 24) & 0xff0000ULL) |
	       ((x >> 40) & 0xff00ULL) |
	        (x >> 56);
}

inline float SwapFloat(float x)
{
	typedef union
	{
		float        f;
		int          i;
		unsigned int ui;
	} floatint_t;

	floatint_t fi;
	fi.f = x;
	fi.i = Swap32(fi.i);
	return fi.f;
}

#ifdef Q3_LITTLE_ENDIAN

#define LittleShort(x) (x)
#define LittleLong(x) (x)
#define Little64(x) (x)
#define LittleFloat(x) (x)
#define BigShort(x) Swap16(x)
#define BigLong(x) Swap32(x)
#define Big64(x) Swap64(x)
#define BigFloat(x) SwapFloat(x)

#elif defined(Q3_BIG_ENDIAN)

#define LittleShort(x) Swap16(x)
#define LittleLong(x) Swap32(x)
#define Little64(x) Swap64(x)
#define LittleFloat(x) SwapFloat(x)
#define BigShort(x) (x)
#define BigLong(x) (x)
#define Big64(x) (x)
#define BigFloat(x) (x)

#else

#error "Unknown endianess"

#endif

#endif // COMMON_ENDIAN_H_
