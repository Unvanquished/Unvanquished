/* rsa2sexp.c
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

#include "rsa.h"

#include "sexp.h"

int
rsa_keypair_to_sexp(struct nettle_buffer *buffer,
		    const char *algorithm_name,
		    const struct rsa_public_key *pub,
		    const struct rsa_private_key *priv)
{
  if (!algorithm_name)
    algorithm_name = "rsa";
  
  if (priv)
    return sexp_format(buffer,
		       "(private-key(%0s(n%b)(e%b)"
		       "(d%b)(p%b)(q%b)(a%b)(b%b)(c%b)))",
		       algorithm_name, pub->n, pub->e,
		       priv->d, priv->p, priv->q,
		       priv->a, priv->b, priv->c);
  else
    return sexp_format(buffer, "(public-key(%0s(n%b)(e%b)))",
		       algorithm_name, pub->n, pub->e);
}

#endif /* WITH_PUBLIC_KEY */
