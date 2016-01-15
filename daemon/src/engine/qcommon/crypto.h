/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2007-2008 Amanieu d'Antras (amanieu@gmail.com)
Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
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

#include <gmp.h>
#include <nettle/bignum.h>
#include <nettle/rsa.h>
#include <nettle/buffer.h>

// Old versions of nettle have nettle_random_func taking an unsigned parameter
// instead of a size_t parameter. Detect this and use the approriate type.
using NettleLength = std::conditional<std::is_same<nettle_random_func, void(void*, size_t, uint8_t*)>::value, size_t, unsigned>::type;

// Random function used for key generation and encryption
void     qnettle_random( void *ctx, NettleLength length, uint8_t *dst );

#endif /* __CRYPTO_H__ */
