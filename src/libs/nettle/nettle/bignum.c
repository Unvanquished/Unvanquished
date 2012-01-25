/* bignum.c
 *
 * bignum operations that are missing from gmp.
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

#if HAVE_LIBGMP

#include <assert.h>
#include <string.h>

#include "bignum.h"

/* Two's complement negation means that -x = ~x + 1, ~x = -(x+1),
 * and we use that x = ~~x = ~(-x-1).
 *
 * Examples:
 *
 *   x  ~x = -x+1     ~~x = x
 *  -1          0          ff
 *  -2          1          fe
 * -7f         7e          81
 * -80         7f          80
 * -81         80        ff7f
 */

/* Including extra sign bit, if needed. Also one byte for zero. */
unsigned
nettle_mpz_sizeinbase_256_s(const mpz_t x)
{
  if (mpz_sgn(x) >= 0)
    return 1 + mpz_sizeinbase(x, 2) / 8;
  else
    {
      /* We'll output ~~x, so we need as many bits as for ~x */
      unsigned size;
      mpz_t c;

      mpz_init(c);
      mpz_com(c, x); /* Same as c = - x - 1 = |x| + 1 */
      size = 1 + mpz_sizeinbase(c,2) / 8;
      mpz_clear(c);

      return size;
    }
}

unsigned
nettle_mpz_sizeinbase_256_u(const mpz_t x)
{
  return (mpz_sizeinbase(x,2) + 7) / 8;
}

static void
nettle_mpz_to_octets(unsigned length, uint8_t *s,
		     const mpz_t x, uint8_t sign)
{
  uint8_t *dst = s + length - 1;
  unsigned size = mpz_size(x);
  unsigned i;
  
  for (i = 0; i<size; i++)
    {
      mp_limb_t limb = mpz_getlimbn(x, i);
      unsigned j;

      for (j = 0; length && j < sizeof(mp_limb_t); j++)
        {
          *dst-- = sign ^ (limb & 0xff);
          limb >>= 8;
          length--;
	}
    }
  
  if (length)
    memset(s, sign, length);
}

void
nettle_mpz_get_str_256(unsigned length, uint8_t *s, const mpz_t x)
{
  if (!length)
    {
      /* x must be zero */
      assert(!mpz_sgn(x));
      return;
    }

  if (mpz_sgn(x) >= 0)
    {
      assert(nettle_mpz_sizeinbase_256_u(x) <= length);
      nettle_mpz_to_octets(length, s, x, 0);
    }
  else
    {
      mpz_t c;
      mpz_init(c);
      mpz_com(c, x);

      /* FIXME: A different trick is to complement all the limbs of c
       * now. That way, nettle_mpz_to_octets need not complement each
       * digit. */
      assert(nettle_mpz_sizeinbase_256_u(c) <= length);
      nettle_mpz_to_octets(length, s, c, 0xff);

      mpz_clear(c);
    }
}

/* Converting from strings */

#ifdef mpz_import
/* Was introduced in GMP-4.1 */
# define nettle_mpz_from_octets(x, length, s) \
   mpz_import((x), (length), 1, 1, 0, 0, (s))
#else
static void
nettle_mpz_from_octets(mpz_t x,
		       unsigned length, const uint8_t *s)
{
  unsigned i;

  mpz_set_ui(x, 0);

  for (i = 0; i < length; i++)
    {
      mpz_mul_2exp(x, x, 8);
      mpz_add_ui(x, x, s[i]);
    }
}
#endif

void
nettle_mpz_set_str_256_u(mpz_t x,
			 unsigned length, const uint8_t *s)
{
  nettle_mpz_from_octets(x, length, s);
}

void
nettle_mpz_init_set_str_256_u(mpz_t x,
			      unsigned length, const uint8_t *s)
{
  mpz_init(x);
  nettle_mpz_from_octets(x, length, s);
}

void
nettle_mpz_set_str_256_s(mpz_t x,
			 unsigned length, const uint8_t *s)
{
  if (!length)
    {
      mpz_set_ui(x, 0);
      return;
    }
  
  nettle_mpz_from_octets(x, length, s);

  if (s[0] & 0x80)
    {
      mpz_t t;

      mpz_init_set_ui(t, 1);
      mpz_mul_2exp(t, t, length*8);
      mpz_sub(x, x, t);
      mpz_clear(t);
    }
}

void
nettle_mpz_init_set_str_256_s(mpz_t x,
			      unsigned length, const uint8_t *s)
{
  mpz_init(x);
  nettle_mpz_set_str_256_s(x, length, s);
}

#endif /* HAVE_LIBGMP */
