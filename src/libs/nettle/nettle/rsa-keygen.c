/* rsa-keygen.c
 *
 * Generation of RSA keypairs
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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "rsa.h"
#include "bignum.h"
#include "nettle-internal.h"

#ifndef DEBUG
# define DEBUG 0
#endif

#if DEBUG
# include <stdio.h>
#endif


#define NUMBER_OF_PRIMES 167

static const unsigned long primes[NUMBER_OF_PRIMES] = {
  3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67,
  71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139,
  149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211,
  223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
  283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367,
  373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443,
  449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523,
  541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613,
  617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691,
  701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787,
  797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877,
  881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971,
  977, 983, 991, 997
};

/* NOTE: The mpz_nextprime in current GMP is unoptimized. */
static void
bignum_next_prime(mpz_t p, mpz_t n, int count,
		  void *progress_ctx, nettle_progress_func progress)
{
  mpz_t tmp;
  TMP_DECL(moduli, unsigned long, NUMBER_OF_PRIMES);
  
  unsigned long difference;
  unsigned prime_limit = NUMBER_OF_PRIMES;
  
  /* First handle tiny numbers */
  if (mpz_cmp_ui(n, 2) <= 0)
    {
      mpz_set_ui(p, 2);
      return;
    }
  mpz_set(p, n);
  mpz_setbit(p, 0);

  if (mpz_cmp_ui(p, 8) < 0)
    return;

  mpz_init(tmp);

  if (mpz_cmp_ui(p, primes[prime_limit-1]) <= 0)
    /* Use only 3, 5 and 7 */
    prime_limit = 3;
  
  /* Compute residues modulo small odd primes */
  TMP_ALLOC(moduli, prime_limit);
  {
    unsigned i;
    for (i = 0; i < prime_limit; i++)
      moduli[i] = mpz_fdiv_ui(p, primes[i]);
  }
  
  for (difference = 0; ; difference += 2)
    {
      int composite = 0;
      unsigned i;

      if (difference >= ULONG_MAX - 10)
	{ /* Should not happen, at least not very often... */
	  mpz_add_ui(p, p, difference);
	  difference = 0;
	}

      /* First check residues */
      for (i = 0; i < prime_limit; i++)
	{
	  if (moduli[i] == 0)
	    composite = 1;
	  moduli[i] = (moduli[i] + 2) % primes[i];
	}
      if (composite)
	continue;
      
      mpz_add_ui(p, p, difference);
      difference = 0;

      if (progress)
	progress(progress_ctx, '.');
      
      /* Fermat test, with respect to 2 */
      mpz_set_ui(tmp, 2);
      mpz_powm(tmp, tmp, p, p);
      if (mpz_cmp_ui(tmp, 2) != 0)
	continue;

      if (progress)
	progress(progress_ctx, '+');

      /* Miller-Rabin test */
      if (mpz_probab_prime_p(p, count))
	break;
    }
  mpz_clear(tmp);
}

/* Returns a random prime of size BITS */
static void
bignum_random_prime(mpz_t x, unsigned bits,
		    void *random_ctx, nettle_random_func random,
		    void *progress_ctx, nettle_progress_func progress)
{
  assert(bits);
  
  for (;;)
    {
      nettle_mpz_random_size(x, random_ctx, random, bits);
      mpz_setbit(x, bits - 1);

      /* Miller-rabin count of 25 is probably much overkill. */
      bignum_next_prime(x, x, 25, progress_ctx, progress);

      if (mpz_sizeinbase(x, 2) == bits)
	break;
    }
}

int
rsa_generate_keypair(struct rsa_public_key *pub,
		     struct rsa_private_key *key,
		     void *random_ctx, nettle_random_func random,
		     void *progress_ctx, nettle_progress_func progress,
		     unsigned n_size,
		     unsigned e_size)
{
  mpz_t p1;
  mpz_t q1;
  mpz_t phi;
  mpz_t tmp;

  if (e_size)
    {
      /* We should choose e randomly. Is the size reasonable? */
      if ((e_size < 16) || (e_size > n_size) )
	return 0;
    }
  else
    {
      /* We have a fixed e. Check that it makes sense */

      /* It must be odd */
      if (!mpz_tstbit(pub->e, 0))
	return 0;

      /* And 3 or larger */
      if (mpz_cmp_ui(pub->e, 3) < 0)
	return 0;
    }
  
  if (n_size < RSA_MINIMUM_N_BITS)
    return 0;
  
  mpz_init(p1); mpz_init(q1); mpz_init(phi); mpz_init(tmp);

  /* Generate primes */
  for (;;)
    {
      /* Generate p, such that gcd(p-1, e) = 1 */
      for (;;)
	{
	  bignum_random_prime(key->p, (n_size+1)/2,
			      random_ctx, random,
			      progress_ctx, progress);
	  mpz_sub_ui(p1, key->p, 1);
      
	  /* If e was given, we must chose p such that p-1 has no factors in
	   * common with e. */
	  if (e_size)
	    break;
	  
	  mpz_gcd(tmp, pub->e, p1);

	  if (mpz_cmp_ui(tmp, 1) == 0)
	    break;
	  else if (progress) progress(progress_ctx, 'c');
	} 

      if (progress)
	progress(progress_ctx, '\n');
      
      /* Generate q, such that gcd(q-1, e) = 1 */
      for (;;)
	{
	  bignum_random_prime(key->q, n_size/2,
			      random_ctx, random,
			      progress_ctx, progress);
	  mpz_sub_ui(q1, key->q, 1);
      
	  /* If e was given, we must chose q such that q-1 has no factors in
	   * common with e. */
	  if (e_size)
	    break;
	  
	  mpz_gcd(tmp, pub->e, q1);

	  if (mpz_cmp_ui(tmp, 1) == 0)
	    break;
	  else if (progress) progress(progress_ctx, 'c');
	}

      /* Now we have the primes. Is the product of the right size? */
      mpz_mul(pub->n, key->p, key->q);
      
      if (mpz_sizeinbase(pub->n, 2) != n_size)
	/* We might get an n of size n_size-1. Then just try again. */
	{
#if DEBUG
	  fprintf(stderr,
		  "\nWanted size: %d, p-size: %d, q-size: %d, n-size: %d\n",
		  n_size,
		  mpz_sizeinbase(key->p,2),
		  mpz_sizeinbase(key->q,2),
		  mpz_sizeinbase(pub->n,2));
#endif
	  if (progress)
	    {
	      progress(progress_ctx, 'b');
	      progress(progress_ctx, '\n');
	    }
	  continue;
	}
      
      if (progress)
	progress(progress_ctx, '\n');

      /* c = q^{-1} (mod p) */
      if (mpz_invert(key->c, key->q, key->p))
	/* This should succeed everytime. But if it doesn't,
	 * we try again. */
	break;
      else if (progress) progress(progress_ctx, '?');
    }

  mpz_mul(phi, p1, q1);
  
  /* If we didn't have a given e, generate one now. */
  if (e_size)
    {
      int retried = 0;
      for (;;)
	{
	  nettle_mpz_random_size(pub->e,
				 random_ctx, random,
				 e_size);
	
	  /* Make sure it's odd and that the most significant bit is
	   * set */
	  mpz_setbit(pub->e, 0);
	  mpz_setbit(pub->e, e_size - 1);

	  /* Needs gmp-3, or inverse might be negative. */
	  if (mpz_invert(key->d, pub->e, phi))
	    break;

	  if (progress) progress(progress_ctx, 'e');
	  retried = 1;
	}
      if (retried && progress)
	progress(progress_ctx, '\n');	
    }
  else
    {
      /* Must always succeed, as we already that e
       * doesn't have any common factor with p-1 or q-1. */
      int res = mpz_invert(key->d, pub->e, phi);
      assert(res);
    }

  /* Done! Almost, we must compute the auxillary private values. */
  /* a = d % (p-1) */
  mpz_fdiv_r(key->a, key->d, p1);

  /* b = d % (q-1) */
  mpz_fdiv_r(key->b, key->d, q1);

  /* c was computed earlier */

  pub->size = key->size = (mpz_sizeinbase(pub->n, 2) + 7) / 8;
  assert(pub->size >= RSA_MINIMUM_N_OCTETS);
  
  mpz_clear(p1); mpz_clear(q1); mpz_clear(phi); mpz_clear(tmp);

  return 1;
}

#endif /* WITH_PUBLIC_KEY */
