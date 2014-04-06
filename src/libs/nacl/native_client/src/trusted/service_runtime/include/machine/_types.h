/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl kernel / service run-time system call ABI.
 * This file defines nacl target machine dependent types.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_MACHINE__TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_MACHINE__TYPES_H_

#ifdef __native_client__
# include <stdint.h>
# include <machine/_default_types.h>
#else
# include "native_client/src/include/portability.h"
#endif

#define __need_size_t
#include <stddef.h>

#ifndef NULL
#define NULL 0
#endif

#define NACL_ABI_WORDSIZE 32

#define NACL_ABI_MAKE_WORDSIZE_TYPE(T)  T ## NACL_ABI_WORDSIZE ## _t
#define NACL_ABI_SIGNED_WORD            NACL_ABI_MAKE_WORDSIZE_TYPE(int)
#define NACL_ABI_UNSIGNED_WORD          NACL_ABI_MAKE_WORDSIZE_TYPE(uint)

#ifndef __native_client__
# if (NACL_ABI_WORDSIZE == NACL_HOST_WORDSIZE)
#   define NACL_ABI_WORDSIZE_IS_NATIVE 1
# else
#   define NACL_ABI_WORDSIZE_IS_NATIVE 0
# endif
#endif

#define NACL_CONCAT3_(a, b, c) a ## b ## c
#ifdef __native_client__
#define NACL_PRI_(fmt, size) NACL_CONCAT3_(PRI, fmt, size)
#else
#define NACL_PRI_(fmt, size) NACL_CONCAT3_(NACL_PRI, fmt, size)
#endif

#ifndef nacl_abi___dev_t_defined
#define nacl_abi___dev_t_defined
typedef int64_t nacl_abi___dev_t;
#ifndef __native_client__
typedef nacl_abi___dev_t nacl_abi_dev_t;
#endif
#endif

#define NACL_PRIdNACL_DEV NACL_PRI_(d, 64)
#define NACL_PRIiNACL_DEV NACL_PRI_(i, 64)
#define NACL_PRIoNACL_DEV NACL_PRI_(o, 64)
#define NACL_PRIuNACL_DEV NACL_PRI_(u, 64)
#define NACL_PRIxNACL_DEV NACL_PRI_(x, 64)
#define NACL_PRIXNACL_DEV NACL_PRI_(X, 64)

#define NACL_ABI_DEV_T_MIN ((nacl_abi_dev_t) 1 << 63)
#define NACL_ABI_DEV_T_MAX (~((nacl_abi_dev_t) 1 << 63))

#ifndef nacl_abi___ino_t_defined
#define nacl_abi___ino_t_defined
typedef uint64_t nacl_abi___ino_t;
#ifndef __native_client__
typedef nacl_abi___ino_t nacl_abi_ino_t;
#endif
#endif

#define NACL_PRIdNACL_INO NACL_PRI_(d, 64)
#define NACL_PRIiNACL_INO NACL_PRI_(i, 64)
#define NACL_PRIoNACL_INO NACL_PRI_(o, 64)
#define NACL_PRIuNACL_INO NACL_PRI_(u, 64)
#define NACL_PRIxNACL_INO NACL_PRI_(x, 64)
#define NACL_PRIXNACL_INO NACL_PRI_(X, 64)

#define NACL_ABI_INO_T_MIN ((nacl_abi_ino_t) 0)
#define NACL_ABI_INO_T_MAX ((nacl_abi_ino_t) -1)

#ifndef nacl_abi___mode_t_defined
#define nacl_abi___mode_t_defined
typedef uint32_t nacl_abi___mode_t;
#ifndef __native_client__
typedef nacl_abi___mode_t nacl_abi_mode_t;
#endif
#endif

#define NACL_PRIdNACL_MODE NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_MODE NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_MODE NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_MODE NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_MODE NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_MODE NACL_PRI_(X, NACL_ABI_WORDSIZE)

#define NACL_ABI_MODE_T_MIN ((nacl_abi_mode_t) 0)
#define NACL_ABI_MODE_T_MAX ((nacl_abi_mode_t) -1)

#ifndef nacl_abi___nlink_t_defined
#define nacl_abi___nlink_t_defined
typedef uint32_t nacl_abi___nlink_t;
#ifndef __native_client__
typedef nacl_abi___nlink_t nacl_abi_nlink_t;
#endif
#endif

#define NACL_PRIdNACL_NLINK NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_NLINK NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_NLINK NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_NLINK NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_NLINK NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_NLINK NACL_PRI_(X, NACL_ABI_WORDSIZE)

#define NACL_ABI_NLINK_T_MIN ((nacl_abi_nlink_t) 0)
#define NACL_ABI_NLINK_T_MAX ((nacl_abi_nlink_t) -1)

#ifndef nacl_abi___uid_t_defined
#define nacl_abi___uid_t_defined
typedef uint32_t nacl_abi___uid_t;
#ifndef __native_client__
typedef nacl_abi___uid_t nacl_abi_uid_t;
#endif
#endif

#define NACL_PRIdNACL_UID NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_UID NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_UID NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_UID NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_UID NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_UID NACL_PRI_(X, NACL_ABI_WORDSIZE)

#define NACL_ABI_UID_T_MIN ((nacl_abi_uid_t) 0)
#define NACL_ABI_UID_T_MAX ((nacl_abi_uid_t) -1)

#ifndef nacl_abi___gid_t_defined
#define nacl_abi___gid_t_defined
typedef uint32_t nacl_abi___gid_t;
#ifndef __native_client__
typedef nacl_abi___gid_t nacl_abi_gid_t;
#endif
#endif

#define NACL_PRIdNACL_GID NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_GID NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_GID NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_GID NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_GID NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_GID NACL_PRI_(X, NACL_ABI_WORDSIZE)

#define NACL_ABI_GID_T_MIN ((nacl_abi_gid_t) 0)
#define NACL_ABI_GID_T_MAX ((nacl_abi_gid_t) -1)

#ifndef nacl_abi___off_t_defined
#define nacl_abi___off_t_defined
typedef int64_t nacl_abi__off_t;
#ifndef __native_client__
typedef nacl_abi__off_t nacl_abi_off_t;
#endif
#endif

#define NACL_PRIdNACL_OFF NACL_PRI_(d, 64)
#define NACL_PRIiNACL_OFF NACL_PRI_(i, 64)
#define NACL_PRIoNACL_OFF NACL_PRI_(o, 64)
#define NACL_PRIuNACL_OFF NACL_PRI_(u, 64)
#define NACL_PRIxNACL_OFF NACL_PRI_(x, 64)
#define NACL_PRIXNACL_OFF NACL_PRI_(X, 64)

#define NACL_ABI_OFF_T_MIN ((nacl_abi_off_t) 1 << 63)
#define NACL_ABI_OFF_T_MAX (~((nacl_abi_off_t) 1 << 63))

#ifndef nacl_abi___off64_t_defined
#define nacl_abi___off64_t_defined
typedef int64_t nacl_abi__off64_t;
#ifndef __native_client__
typedef nacl_abi__off64_t nacl_abi_off64_t;
#endif
#endif

#define NACL_PRIdNACL_OFF64 NACL_PRI_(d, 64)
#define NACL_PRIiNACL_OFF64 NACL_PRI_(i, 64)
#define NACL_PRIoNACL_OFF64 NACL_PRI_(o, 64)
#define NACL_PRIuNACL_OFF64 NACL_PRI_(u, 64)
#define NACL_PRIxNACL_OFF64 NACL_PRI_(x, 64)
#define NACL_PRIXNACL_OFF64 NACL_PRI_(X, 64)

#define NACL_ABI_OFF64_T_MIN ((nacl_abi_off64_t) 1 << 63)
#define NACL_ABI_OFF64_T_MAX (~((nacl_abi_off64_t) 1 << 63))


#if !(defined(__GLIBC__) && defined(__native_client__))

#ifndef nacl_abi___blksize_t_defined
#define nacl_abi___blksize_t_defined
typedef int32_t nacl_abi___blksize_t;
typedef nacl_abi___blksize_t nacl_abi_blksize_t;
#endif

#define NACL_PRIdNACL_BLKSIZE NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_BLKSIZE NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_BLKSIZE NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_BLKSIZE NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_BLKSIZE NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_BLKSIZE NACL_PRI_(X, NACL_ABI_WORDSIZE)

#endif


#define NACL_ABI_BLKSIZE_T_MIN \
  ((nacl_abi_blksize_t) 1 << (NACL_ABI_WORDSIZE - 1))
#define NACL_ABI_BLKSIZE_T_MAX \
  (~((nacl_abi_blksize_t) 1 << (NACL_ABI_WORDSIZE - 1)))

#ifndef nacl_abi___blkcnt_t_defined
#define nacl_abi___blkcnt_t_defined
typedef int32_t nacl_abi___blkcnt_t;
typedef nacl_abi___blkcnt_t nacl_abi_blkcnt_t;
#endif

#define NACL_PRIdNACL_BLKCNT NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_BLKCNT NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_BLKCNT NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_BLKCNT NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_BLKCNT NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_BLKCNT NACL_PRI_(X, NACL_ABI_WORDSIZE)

#define NACL_ABI_BLKCNT_T_MIN \
  ((nacl_abi_blkcnt_t) 1 << (NACL_ABI_WORDSIZE - 1))
#define NACL_ABI_BLKCNT_T_MAX \
  (~((nacl_abi_blkcnt_t) 1 << (NACL_ABI_WORDSIZE - 1)))

#ifndef nacl_abi___time_t_defined
#define nacl_abi___time_t_defined
typedef int64_t       nacl_abi___time_t;
typedef nacl_abi___time_t nacl_abi_time_t;
#endif

#ifndef nacl_abi___timespec_defined
#define nacl_abi___timespec_defined
struct nacl_abi_timespec {
  nacl_abi_time_t tv_sec;
#ifdef __native_client__
  long int        tv_nsec;
#else
  int32_t         tv_nsec;
#endif
};
#endif

#define NACL_PRIdNACL_TIME NACL_PRI_(d, 64)
#define NACL_PRIiNACL_TIME NACL_PRI_(i, 64)
#define NACL_PRIoNACL_TIME NACL_PRI_(o, 64)
#define NACL_PRIuNACL_TIME NACL_PRI_(u, 64)
#define NACL_PRIxNACL_TIME NACL_PRI_(x, 64)
#define NACL_PRIXNACL_TIME NACL_PRI_(X, 64)

#define NACL_ABI_TIME_T_MIN ((nacl_abi_time_t) 1 << 63)
#define NACL_ABI_TIME_T_MAX (~((nacl_abi_time_t) 1 << 63))

/*
 * stddef.h defines size_t, and we cannot export another definition.
 * see __need_size_t above and stddef.h
 * (BUILD/gcc-4.2.2/gcc/ginclude/stddef.h) contents.
 */
#define NACL_NO_STRIP(t) nacl_ ## abi_ ## t

/*
 * NO_STRIP blocks stripping of NACL_ABI_ from any symbols between the
 * #define and #undef.  This causes NACL_ABI_WORDSIZE to be undefined below
 * in the NACL_PRI?NACL_SIZE macros.  If it is not defined and we are in
 * __native_client__, then define it in terms of WORDSIZE, which is the
 * stripped result of the #define above.
 */
#if defined(__native_client__) && !defined(NACL_ABI_WORDSIZE)
#define NACL_ABI_WORDSIZE WORDSIZE
#endif  /* defined(__native_client__) && !defined(NACL_ABI_WORDSIZE) */

#ifndef nacl_abi_size_t_defined
#define nacl_abi_size_t_defined
typedef uint32_t NACL_NO_STRIP(size_t);
#endif

#define NACL_ABI_SIZE_T_MIN ((nacl_abi_size_t) 0)
#define NACL_ABI_SIZE_T_MAX ((nacl_abi_size_t) -1)

#ifndef nacl_abi_ssize_t_defined
#define nacl_abi_ssize_t_defined
typedef int32_t NACL_NO_STRIP(ssize_t);
#endif

#define NACL_ABI_SSIZE_T_MAX \
  ((nacl_abi_ssize_t) (NACL_ABI_SIZE_T_MAX >> 1))
#define NACL_ABI_SSIZE_T_MIN \
  (~NACL_ABI_SSIZE_T_MAX)

#define NACL_PRIdNACL_SIZE NACL_PRI_(d, NACL_ABI_WORDSIZE)
#define NACL_PRIiNACL_SIZE NACL_PRI_(i, NACL_ABI_WORDSIZE)
#define NACL_PRIoNACL_SIZE NACL_PRI_(o, NACL_ABI_WORDSIZE)
#define NACL_PRIuNACL_SIZE NACL_PRI_(u, NACL_ABI_WORDSIZE)
#define NACL_PRIxNACL_SIZE NACL_PRI_(x, NACL_ABI_WORDSIZE)
#define NACL_PRIXNACL_SIZE NACL_PRI_(X, NACL_ABI_WORDSIZE)

/**
 * Inline functions to aid in conversion between system (s)size_t and
 * nacl_abi_(s)size_t
 *
 * These are defined as inline functions only if __native_client__ is
 * undefined, since in a nacl module size_t and nacl_abi_size_t are always
 * the same (and we don't have a definition for INLINE, so these won't compile)
 *
 * If __native_client__ *is* defined, these turn into no-ops.
 */
#ifdef __native_client__
  /**
   * NB: The "no-op" version of these functions does NO type conversion.
   * Please DO NOT CHANGE THIS. If you get a type error using these functions
   * in a NaCl module, it's a real error and should be fixed in your code.
   */
  #define nacl_abi_size_t_saturate(x) (x)
  #define nacl_abi_ssize_t_saturate(x) (x)
#else /* __native_client */
  static INLINE nacl_abi_size_t nacl_abi_size_t_saturate(size_t x) {
    if (x > NACL_ABI_SIZE_T_MAX) {
      return NACL_ABI_SIZE_T_MAX;
    } else {
      return (nacl_abi_size_t)x;
    }
  }

  static INLINE nacl_abi_ssize_t nacl_abi_ssize_t_saturate(ssize_t x) {
    if (x > NACL_ABI_SSIZE_T_MAX) {
      return NACL_ABI_SSIZE_T_MAX;
    } else if (x < NACL_ABI_SSIZE_T_MIN) {
      return NACL_ABI_SSIZE_T_MIN;
    } else {
      return (nacl_abi_ssize_t) x;
    }
  }
#endif /* __native_client */

#undef NACL_NO_STRIP

#endif
