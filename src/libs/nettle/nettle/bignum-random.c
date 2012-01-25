/* bignum-random.c
 *
 * Generating big random numbers
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels Möller
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

#if HAVE_LIBGMP

#include <stdlib.h>

#include "bignum.h"
#include "nettle-internal.h"

void
nettle_mpz_random_size(mpz_t x,
		       void *ctx, nettle_random_func random,
		       unsigned bits)
{
  unsigned length = (bits + 7) / 8;
  TMP_DECL(data, uint8_t, NETTLE_MAX_BIGNUM_BITS / 8);
  TMP_ALLOC(data, length);

  random(ctx, length, data);

  nettle_mpz_set_str_256_u(x, length, data);

  if (bits % 8)
    mpz_fdiv_r_2exp(x, x, bits);
}

/* Returns a random number x, 0 <= x < n */
void
nettle_mpz_random(mpz_t x,
		  void *ctx, nettle_random_func random,
		  const mpz_t n)
{
  /* FIXME: This leaves some bias, which may be bad for DSA. A better
   * way might to generate a random number of mpz_sizeinbase(n, 2)
   * bits, and loop until one smaller than n is found. */

  /* From Daniel Bleichenbacher (via coderpunks):
   *
   * There is still a theoretical attack possible with 8 extra bits.
   * But, the attack would need about 2^66 signatures 2^66 memory and
   * 2^66 time (if I remember that correctly). Compare that to DSA,
   * where the attack requires 2^22 signatures 2^40 memory and 2^64
   * time. And of course, the numbers above are not a real threat for
   * PGP. Using 16 extra bits (i.e. generating a 176 bit random number
   * and reducing it modulo q) will defeat even this theoretical
   * attack.
   * 
   * More generally log_2(q)/8 extra bits are enough to defeat my
   * attack. NIST also plans to update the standard.
   */

  /* Add a few bits extra, to decrease the bias from the final modulo
   * operation. */

  nettle_mpz_random_size(x, 
			 ctx, random,
			 mpz_sizeinbase(n, 2) + 16);
  
  mpz_fdiv_r(x, x, n);
}

#endif /* HAVE_LIBGMP */
