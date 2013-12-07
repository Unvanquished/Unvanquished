/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl kernel / service run-time system call ABI.  stat/fstat.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_STAT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_STAT_H_

#ifdef __native_client__
#include <sys/types.h>
#else
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"
#endif
#include "native_client/src/trusted/service_runtime/include/bits/stat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Linux <bit/stat.h> uses preprocessor to define st_atime to
 * st_atim.tv_sec etc to widen the ABI to use a struct timespec rather
 * than just have a time_t access/modification/inode-change times.
 * this is unfortunate, since that symbol cannot be used as a struct
 * member elsewhere (!).
 *
 * just like with type name conflicts, we avoid it by using nacl_abi_
 * as a prefix for struct members too.  sigh.
 */

struct nacl_abi_stat {  /* must be renamed when ABI is exported */
  nacl_abi_dev_t     nacl_abi_st_dev;       /* not implemented */
  nacl_abi_ino_t     nacl_abi_st_ino;       /* not implemented */
  nacl_abi_mode_t    nacl_abi_st_mode;      /* partially implemented. */
  nacl_abi_nlink_t   nacl_abi_st_nlink;     /* link count */
  nacl_abi_uid_t     nacl_abi_st_uid;       /* not implemented */
  nacl_abi_gid_t     nacl_abi_st_gid;       /* not implemented */
  nacl_abi_dev_t     nacl_abi_st_rdev;      /* not implemented */
  nacl_abi_off_t     nacl_abi_st_size;      /* object size */
  nacl_abi_blksize_t nacl_abi_st_blksize;   /* not implemented */
  nacl_abi_blkcnt_t  nacl_abi_st_blocks;    /* not implemented */
  nacl_abi_time_t    nacl_abi_st_atime;     /* access time */
  int64_t            nacl_abi_st_atimensec; /* possibly just pad */
  nacl_abi_time_t    nacl_abi_st_mtime;     /* modification time */
  int64_t            nacl_abi_st_mtimensec; /* possibly just pad */
  nacl_abi_time_t    nacl_abi_st_ctime;     /* inode change time */
  int64_t            nacl_abi_st_ctimensec; /* possibly just pad */
};

#ifdef __native_client__
extern int stat(char const *path, struct nacl_abi_stat *stbuf);
extern int fstat(int d, struct nacl_abi_stat *stbuf);
extern int lstat(const char *path, struct nacl_abi_stat *stbuf);
extern int mkdir(const char *path, nacl_abi_mode_t mode);
#endif

#ifdef __cplusplus
}
#endif

#endif
