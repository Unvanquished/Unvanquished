/* rsa.c
 *
 * The RSA publickey algorithm.
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

#include "rsa.h"

#include "bignum.h"

void
rsa_public_key_init(struct rsa_public_key *key)
{
  mpz_init(key->n);
  mpz_init(key->e);

  /* Not really necessary, but it seems cleaner to initialize all the
   * storage. */
  key->size = 0;
}

void
rsa_public_key_clear(struct rsa_public_key *key)
{
  mpz_clear(key->n);
  mpz_clear(key->e);
}

/* Computes the size, in octets, of a the modulo. Returns 0 if the
 * modulo is too small to be useful. */

unsigned
_rsa_check_size(mpz_t n)
{
  /* Round upwards */
  unsigned size = (mpz_sizeinbase(n, 2) + 7) / 8;

  if (size < RSA_MINIMUM_N_OCTETS)
    return 0;

  return size;
}

int
rsa_public_key_prepare(struct rsa_public_key *key)
{
  /* FIXME: Add further sanity checks, like 0 < e < n. */
#if 0
  if ( (mpz_sgn(key->e) <= 0)
       || mpz_cmp(key->e, key->n) >= 0)
    return 0;
#endif
  
  key->size = _rsa_check_size(key->n);
  
  return (key->size > 0);
}

#endif /* WITH_PUBLIC_KEY */
