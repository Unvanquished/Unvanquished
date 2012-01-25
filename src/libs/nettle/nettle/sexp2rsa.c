/* sexp2rsa.c
 *
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

#if WITH_PUBLIC_KEY

#include <string.h>

#include "rsa.h"

#include "bignum.h"
#include "sexp.h"

#define GET(x, l, v)				\
do {						\
  if (!nettle_mpz_set_sexp((x), (l), (v))	\
      || mpz_sgn(x) <= 0)			\
    return 0;					\
} while(0)

/* Iterator should point past the algorithm tag, e.g.
 *
 *   (public-key (rsa (n |xxxx|) (e |xxxx|))
 *                    ^ here
 */

int
rsa_keypair_from_sexp_alist(struct rsa_public_key *pub,
			    struct rsa_private_key *priv,
			    unsigned limit,
			    struct sexp_iterator *i)
{
  static const uint8_t * const names[8]
    = { "n", "e", "d", "p", "q", "a", "b", "c" };
  struct sexp_iterator values[8];
  unsigned nvalues = priv ? 8 : 2;
  
  if (!sexp_iterator_assoc(i, nvalues, names, values))
    return 0;

  if (priv)
    {
      GET(priv->d, limit, &values[2]);
      GET(priv->p, limit, &values[3]);
      GET(priv->q, limit, &values[4]);
      GET(priv->a, limit, &values[5]);
      GET(priv->b, limit, &values[6]);
      GET(priv->c, limit, &values[7]);

      if (!rsa_private_key_prepare(priv))
	return 0;
    }

  if (pub)
    {
      GET(pub->n, limit, &values[0]);
      GET(pub->e, limit, &values[1]);

      if (!rsa_public_key_prepare(pub))
	return 0;
    }
  
  return 1;
}

int
rsa_keypair_from_sexp(struct rsa_public_key *pub,
		      struct rsa_private_key *priv,
		      unsigned limit, 
		      unsigned length, const uint8_t *expr)
{
  struct sexp_iterator i;
  static const uint8_t * const names[3]
    = { "rsa", "rsa-pkcs1", "rsa-pkcs1-sha1" };

  if (!sexp_iterator_first(&i, length, expr))
    return 0;
  
  if (!sexp_iterator_check_type(&i, priv ? "private-key" : "public-key"))
    return 0;

  if (!sexp_iterator_check_types(&i, 3, names))
    return 0;

  return rsa_keypair_from_sexp_alist(pub, priv, limit, &i);
}

#endif /* WITH_PUBLIC_KEY */
