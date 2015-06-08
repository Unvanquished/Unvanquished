/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.  Time types.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_TIME_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_TIME_H_

#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __native_client__
#ifndef nacl_abi___susecond_t_defined
#define nacl_abi___susecond_t_defined
typedef long int  nacl_abi_suseconds_t;
#endif
#else
typedef int32_t   nacl_abi_suseconds_t;
#endif

#ifndef nacl_abi___clock_t_defined
#define nacl_abi___clock_t_defined
typedef long int  nacl_abi_clock_t;  /* to be deprecated */
#endif

struct nacl_abi_timeval {
  nacl_abi_time_t      nacl_abi_tv_sec;
  nacl_abi_suseconds_t nacl_abi_tv_usec;
};

/* obsolete.  should not be used */
struct nacl_abi_timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

/*
 * In some places (e.g., the linux man page) the second parameter is defined
 * as a struct timezone *.  The header file says this struct type should
 * never be used, and defines it by default as void *.  The Mac man page says
 * it is void *.
 */
extern int nacl_abi_gettimeofday(struct nacl_abi_timeval *tv, void *tz);

/*
 * POSIX defined clock id values for clock_getres andn clock_gettime.
 */

#define NACL_ABI_CLOCK_REALTIME           (0)
#define NACL_ABI_CLOCK_MONOTONIC          (1)
#define NACL_ABI_CLOCK_PROCESS_CPUTIME_ID (2)
#define NACL_ABI_CLOCK_THREAD_CPUTIME_ID  (3)

#ifdef __cplusplus
}
#endif

#endif
