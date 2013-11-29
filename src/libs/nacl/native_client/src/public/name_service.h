/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _NATIVE_CLIENT_SRC_PUBLIC_NAME_SERVICE_H_
#define _NATIVE_CLIENT_SRC_PUBLIC_NAME_SERVICE_H_

#define NACL_NAME_SERVICE_CONNECTION_MAX  8

#define NACL_NAME_SERVICE_SUCCESS                 0
#define NACL_NAME_SERVICE_NAME_NOT_FOUND          1
#define NACL_NAME_SERVICE_DUPLICATE_NAME          2
#define NACL_NAME_SERVICE_INSUFFICIENT_RESOURCES  3
#define NACL_NAME_SERVICE_PERMISSION_DENIED       4
#define NACL_NAME_SERVICE_INVALID_ARGUMENT        5

#define NACL_NAME_SERVICE_LIST            "list::C"
/* output buffer contains NUL separated names */

#define NACL_NAME_SERVICE_INSERT          "insert:sih:i"
/* object name, max access mode, descriptor -> status */

#define NACL_NAME_SERVICE_DELETE          "delete:s:i"
/* object name -> status */

#define NACL_NAME_SERVICE_LOOKUP_DEPRECATED "lookup:s:ih"
/*
 * object name -> status, descriptor; flags curried to be RDONLY.
 * deprecated; for ABI backward compatibility while TC and apps are
 * updated.
 */

#define NACL_NAME_SERVICE_LOOKUP          "lookup:si:ih"
/* object name, flags -> status, descriptor */

#define NACL_NAME_SERVICE_TAKE_OWNERSHIP  "take_ownership::"
/*
 * make the name service immutable by all other connections.  when the
 * owner connection is closed, the name service may become writable to
 * all again.  does not have to be implemented by all name services.
 */

/*
 * Mode argument for insert and flags argument lookup must contain in
 * the NACL_ABI_O_ACCMODE bit field one of
 *
 * NACL_ABI_O_RDONLY
 * NACL_ABI_O_WRONLY
 * NACL_ABI_O_RDWR
 *
 * and lookup flags argument may also contain any of
 *
 * NACL_ABI_O_CREAT
 * NACL_ABI_O_TRUNC
 * NACL_ABI_O_APPEND
 *
 * though whether the latter flag bits are supported or not depends on
 * the name service.
 *
 * Connection capabilities should be looked up with O_RDWR.  An fstat
 * is required if client code needs to distinguish between a (file)
 * descriptor for which the read and write syscalls work and a
 * connection capability, though it's better to just attempt the
 * syscall(s) and detect the failure.
 *
 * When lookup occurs and asks for RDONLY, if a name entry with that
 * name with either RDONLY or RDWR mode exists, then that entry is
 * returned; otherwise a NACL_NAME_SERVICE_NAME_NOT_FOUND or
 * NACL_NAME_SERVICE_PERMISSION_DENIED status is returned.
 */

#endif  /* _NATIVE_CLIENT_SRC_PUBLIC_NAME_SERVICE_H_ */
