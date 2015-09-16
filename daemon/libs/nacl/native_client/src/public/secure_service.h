/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_SECURE_SERVICE_H_
#define NATIVE_CLIENT_SRC_PUBLIC_SECURE_SERVICE_H_

#define NACL_SECURE_SERVICE_LOAD_MODULE     "load_module:hs:"
/* nexe to load, auxiliary vector */

#define NACL_SECURE_SERVICE_START_MODULE    "start_module::i"
/* -> load module status */

#define NACL_SECURE_SERVICE_REVERSE_SETUP   "reverse_setup::h"
/* -> reverse service address */

#define NACL_SECURE_SERVICE_LOG             "log:is:"
/* severity and message to log */

#define NACL_SECURE_SERVICE_HARD_SHUTDOWN   "hard_shutdown::"
/* shutdown module */

#endif /* NATIVE_CLIENT_SRC_PUBLIC_SECURE_SERVICE_H_ */
