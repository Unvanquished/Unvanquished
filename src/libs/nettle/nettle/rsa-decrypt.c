/* rsa_decrypt.c
 *
 * The RSA publickey algorithm. PKCS#1 encryption.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2001 Niels Möller
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

#include "config.h"

#if WITH_PUBLIC_KEY

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "rsa.h"

#include "bignum.h"
#include "nettle-internal.h"

int
rsa_decrypt(const struct rsa_private_key *key,
	    unsigned *length, uint8_t *message,
	    const mpz_t gibberish)
{
  TMP_DECL(em, uint8_t, NETTLE_MAX_BIGNUM_BITS / 8);
  uint8_t *terminator;
  unsigned padding;
  unsigned message_length;
  
  mpz_t m;

  mpz_init(m);
  rsa_compute_root(key, m, gibberish);

  TMP_ALLOC(em, key->size);
  nettle_mpz_get_str_256(key->size, em, m);
  mpz_clear(m);

  /* Check format */
  if (em[0] || em[1] != 2)
    return 0;

  terminator = memchr(em + 2, 0, key->size - 2);

  if (!terminator)
    return 0;
  
  padding = terminator - (em + 2);
  if (padding < 8)
    return 0;

  message_length = key->size - 3 - padding;

  if (*length < message_length)
    return 0;
  
  memcpy(message, terminator + 1, message_length);
  *length = message_length;

  return 1;
}

#endif /* WITH_PUBLIC_KEY */
