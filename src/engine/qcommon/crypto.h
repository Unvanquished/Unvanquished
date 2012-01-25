/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 2007-2008 Amanieu d'Antras (amanieu@gmail.com)
Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>

OpenWolf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OpenWolf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include "q_shared.h"
#include "qcommon.h"

#ifdef USE_CRYPTO
#include <gmp.h>
#include <nettle/bignum.h>
#include <nettle/rsa.h>
#include <nettle/buffer.h>

qboolean Crypto_Init(void);
void Crypto_Shutdown(void);
// The size is stored in the location pointed to by second arg, and will change as the buffer grows
void qnettle_buffer_init(struct nettle_buffer *buffer, int *size);
// Random function used for key generation and encryption
void qnettle_random(void *ctx, unsigned length, uint8_t *dst);
#else
#define Crypto_Init() qfalse
#define Crypto_Shutdown()
#endif

#endif /* __CRYPTO_H__ */
