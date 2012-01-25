/* nettle-types.h */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2005 Niels Möller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef NETTLE_TYPES_H
#define NETTLE_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Uses a void * for cipher contexts.

   For block ciphers it would make sense with a const void * for the
   context, but we use the same typedef for stream ciphers where the
   internal state changes during the encryption. */

typedef void (*nettle_crypt_func)(void *ctx,
	      unsigned length, uint8_t *dst,
	      const uint8_t *src);

/* FIXME: Move more of the typedefs to this file? */

#ifdef __cplusplus
}
#endif

#endif /* NETTLE_TYPES_H */
