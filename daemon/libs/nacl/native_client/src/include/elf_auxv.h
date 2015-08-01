/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_ELF_AUXV_H_
#define NATIVE_CLIENT_SRC_INCLUDE_ELF_AUXV_H_ 1

/*
 * This is only in a separate file from elf.h and elf_constants.h
 * because these two contains #ifs which depend on
 * architecture-specific #defines which are supplied by the build
 * system.  These #defines are not normally set when building
 * untrusted code.
 */

/* Keys for auxiliary vector (auxv). */
#define AT_NULL         0   /* Terminating item in auxv array */
#define AT_ENTRY        9   /* Entry point of the executable */
#define AT_SYSINFO      32  /* System call entry point */

#endif
