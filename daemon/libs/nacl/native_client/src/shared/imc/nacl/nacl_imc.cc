/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NaCl inter-module communication primitives. */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>

#include "native_client/src/shared/imc/nacl_imc_c.h"

#if defined(__native_client__)
#include "native_client/src/public/imc_syscalls.h"
typedef NaClAbiNaClImcMsgHdr NaClImcMsgHdr;
#endif

/* Duplicate a NaCl file descriptor. */
NaClHandle NaClDuplicateNaClHandle(NaClHandle handle) {
  return dup(handle);
}

int NaClWouldBlock() {
  return (errno == EAGAIN) ? 1 : 0;
}

int NaClGetLastErrorString(char* buffer, size_t length) {
  char* message;
  /* Note newlib provides only GNU version of strerror_r(). */
  if (buffer == NULL || length == 0) {
    errno = ERANGE;
    return -1;
  }
  message = strerror_r(errno, buffer, length);
  if (message != buffer) {
    size_t message_bytes = strlen(message) + 1;
    length = std::min(message_bytes, length);
    memmove(buffer, message, length);
    buffer[length - 1] = '\0';
  }
  return 0;
}

NaClHandle NaClBoundSocket(const NaClSocketAddress* address) {
  /*
   * TODO(shiki): Switch to the following once the make_bound_sock() prototype
   *              is cleaned up.
   * return make_bound_sock(address);
   */
  return -1;
}

int NaClSocketPair(NaClHandle pair[2]) {
  return imc_socketpair(pair);
}

int NaClClose(NaClHandle handle) {
  return close(handle);
}

int NaClSendDatagram(NaClHandle handle, const NaClMessageHeader* message,
                     int flags) {
  return imc_sendmsg(handle, (const NaClImcMsgHdr *) message,
                     flags);
}

int NaClSendDatagramTo(const NaClMessageHeader* message, int flags,
                       const NaClSocketAddress* name) {
  return -1;  /* TODO(bsy): how to implement this for NaCl? */
}

int NaClReceiveDatagram(NaClHandle handle, NaClMessageHeader* message,
                        int flags) {
  return imc_recvmsg(handle, (NaClImcMsgHdr *) message, flags);
}

NaClHandle NaClCreateMemoryObject(size_t length, int executable) {
  if (executable) {
    return -1;  /* Will never work with NaCl and should never be invoked. */
  }
  return imc_mem_obj_create(length);
}

void* NaClMap(struct NaClDescEffector* effp,
              void* start, size_t length, int prot, int flags,
              NaClHandle memory, off_t offset) {
  static int posix_prot[4] = {
    PROT_NONE,
    PROT_READ,
    PROT_WRITE,
    PROT_READ | PROT_WRITE
  };
  int adjusted = 0;

  if (flags & NACL_MAP_SHARED) {
    adjusted |= MAP_SHARED;
  }
  if (flags & NACL_MAP_PRIVATE) {
    adjusted |= MAP_PRIVATE;
  }
  if (flags & NACL_MAP_FIXED) {
    adjusted |= MAP_FIXED;
  }
  return mmap(start, length, posix_prot[prot & 3], adjusted, memory, offset);
}

int NaClUnmap(void* start, size_t length) {
  return munmap(start, length);
}
