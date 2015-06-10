/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Wrapper for syscall.
 */

#include <errno.h>
#include <sys/types.h>

#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

int imc_socketpair(int *d2) {
  int retval;
  retval = NACL_SYSCALL(imc_socketpair)(d2);

  if (retval < 0) {
    errno = -retval;
    return -1;
  }
  return retval;
}
