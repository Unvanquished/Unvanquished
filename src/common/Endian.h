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
// Adroid specific code, see: https://svn.boost.org/trac/boost/ticket/7528
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
#elif defined(Q3_VM)
#else
# error The file boost/detail/endian.hpp needs to be set up for your CPU type.
#endif

// Compilers are smart enough to optimize these to a single bswap instruction
STATIC_INLINE int16_t Swap16(int16_t x) IFDECLARE
#ifdef Q3_VM_INSTANTIATE
{
	return (x >> 8) | (x << 8);
}
#endif

STATIC_INLINE int32_t Swap32(int32_t x) IFDECLARE
#ifdef Q3_VM_INSTANTIATE
{
	return (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | (x >> 24);
}
#endif

#ifndef Q3_VM
STATIC_INLINE int64_t Swap64(int64_t x)
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
#endif

STATIC_INLINE float SwapFloat(float x) IFDECLARE
#ifdef Q3_VM_INSTANTIATE
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
#endif

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

#elif defined(Q3_VM)

#define LittleShort(x) (x)
#define LittleLong(x) (x)
#define Little64(x) (x)
#define LittleFloat(x) (x)
#define BigShort(x) (x)
#define BigLong(x) (x)
#define Big64(x) (x)
#define BigFloat(x) (x)

#else

#error "Unknown endianess"

#endif

#endif // COMMON_ENDIAN_H_
