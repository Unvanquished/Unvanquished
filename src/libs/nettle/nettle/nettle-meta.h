/* nettle-meta.h
 *
 * Information about algorithms.
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

#ifndef NETTLE_META_H_INCLUDED
#define NETTLE_META_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Randomness. Used by key generation and dsa signature creation. */
typedef void (*nettle_random_func)(void *ctx,
				   unsigned length, uint8_t *dst);

/* Progress report function, mainly for key generation. */
typedef void (*nettle_progress_func)(void *ctx,
				     int c);

/* Ciphers */
typedef void (*nettle_set_key_func)(void *ctx,
				    unsigned length,
				    const uint8_t *key);


struct nettle_cipher
{
  const char *name;
  
  unsigned context_size;
  
  /* Zero for stream ciphers */
  unsigned block_size;

  /* Suggested key size; other sizes are sometimes possible. */
  unsigned key_size;

  nettle_set_key_func set_encrypt_key;
  nettle_set_key_func set_decrypt_key;

  nettle_crypt_func encrypt;
  nettle_crypt_func decrypt;
};

#define _NETTLE_CIPHER(name, NAME, keysize) {	\
  #name #keysize,				\
  sizeof(struct name##_ctx),			\
  NAME##_BLOCK_SIZE,				\
  keysize / 8,					\
  (nettle_set_key_func) name##_set_key,		\
  (nettle_set_key_func) name##_set_key,		\
  (nettle_crypt_func) name##_encrypt,		\
  (nettle_crypt_func) name##_decrypt,		\
}

#define _NETTLE_CIPHER_SEP(name, NAME, keysize) {	\
  #name #keysize,				\
  sizeof(struct name##_ctx),			\
  NAME##_BLOCK_SIZE,				\
  keysize / 8,					\
  (nettle_set_key_func) name##_set_encrypt_key,	\
  (nettle_set_key_func) name##_set_decrypt_key,	\
  (nettle_crypt_func) name##_encrypt,		\
  (nettle_crypt_func) name##_decrypt,		\
}

#define _NETTLE_CIPHER_FIX(name, NAME, keysize) {	\
  #name,					\
  sizeof(struct name##_ctx),			\
  NAME##_BLOCK_SIZE,				\
  keysize / 8,					\
  (nettle_set_key_func) name##_set_key,		\
  (nettle_set_key_func) name##_set_key,		\
  (nettle_crypt_func) name##_encrypt,		\
  (nettle_crypt_func) name##_decrypt,		\
}

extern const struct nettle_cipher nettle_aes128;
extern const struct nettle_cipher nettle_aes192;
extern const struct nettle_cipher nettle_aes256;

extern const struct nettle_cipher nettle_arcfour128;
extern const struct nettle_cipher nettle_cast128;

extern const struct nettle_cipher nettle_serpent128;
extern const struct nettle_cipher nettle_serpent192;
extern const struct nettle_cipher nettle_serpent256;

extern const struct nettle_cipher nettle_twofish128;
extern const struct nettle_cipher nettle_twofish192;
extern const struct nettle_cipher nettle_twofish256;

extern const struct nettle_cipher nettle_arctwo40;
extern const struct nettle_cipher nettle_arctwo64;
extern const struct nettle_cipher nettle_arctwo128;
extern const struct nettle_cipher nettle_arctwo_gutmann128;


/* Hash algorithms */
typedef void (*nettle_hash_init_func)(void *ctx);
typedef void (*nettle_hash_update_func)(void *ctx,
					unsigned length,
					const uint8_t *src);
typedef void (*nettle_hash_digest_func)(void *ctx,
					unsigned length, uint8_t *dst);

struct nettle_hash
{
  const char *name;

  /* Size of the context struct */
  unsigned context_size;

  /* Size of digests */
  unsigned digest_size;
  
  /* Internal block size */
  unsigned block_size;

  nettle_hash_init_func init;
  nettle_hash_update_func update;
  nettle_hash_digest_func digest;
};

#define _NETTLE_HASH(name, NAME) {		\
 #name,						\
 sizeof(struct name##_ctx),			\
 NAME##_DIGEST_SIZE,				\
 NAME##_DATA_SIZE,				\
 (nettle_hash_init_func) name##_init,		\
 (nettle_hash_update_func) name##_update,	\
 (nettle_hash_digest_func) name##_digest	\
} 

extern const struct nettle_hash nettle_md2;
extern const struct nettle_hash nettle_md4;
extern const struct nettle_hash nettle_md5;
extern const struct nettle_hash nettle_sha1;
extern const struct nettle_hash nettle_sha256;


/* ASCII armor codecs. NOTE: Experimental and subject to change. */

typedef unsigned (*nettle_armor_length_func)(unsigned length);
typedef void (*nettle_armor_init_func)(void *ctx);

typedef unsigned (*nettle_armor_encode_update_func)(void *ctx,
						    uint8_t *dst,
						    unsigned src_length,
						    const uint8_t *src);

typedef unsigned (*nettle_armor_encode_final_func)(void *ctx, uint8_t *dst);

typedef int (*nettle_armor_decode_update_func)(void *ctx,
					       unsigned *dst_length,
					       uint8_t *dst,
					       unsigned src_length,
					       const uint8_t *src);

typedef int (*nettle_armor_decode_final_func)(void *ctx);

struct nettle_armor
{
  const char *name;
  unsigned encode_context_size;
  unsigned decode_context_size;

  unsigned encode_final_length;

  nettle_armor_init_func encode_init;
  nettle_armor_length_func encode_length;
  nettle_armor_encode_update_func encode_update;
  nettle_armor_encode_final_func encode_final;
  
  nettle_armor_init_func decode_init;
  nettle_armor_length_func decode_length;
  nettle_armor_decode_update_func decode_update;
  nettle_armor_decode_final_func decode_final;
};

#define _NETTLE_ARMOR(name, NAME) {				\
  #name,							\
  sizeof(struct name##_encode_ctx),				\
  sizeof(struct name##_decode_ctx),				\
  NAME##_ENCODE_FINAL_LENGTH,					\
  (nettle_armor_init_func) name##_encode_init,			\
  (nettle_armor_length_func) name##_encode_length,		\
  (nettle_armor_encode_update_func) name##_encode_update,	\
  (nettle_armor_encode_final_func) name##_encode_final,		\
  (nettle_armor_init_func) name##_decode_init,			\
  (nettle_armor_length_func) name##_decode_length,		\
  (nettle_armor_decode_update_func) name##_decode_update,	\
  (nettle_armor_decode_final_func) name##_decode_final,		\
}

#define _NETTLE_ARMOR_0(name, NAME) {				\
  #name,							\
  0,								\
  sizeof(struct name##_decode_ctx),				\
  NAME##_ENCODE_FINAL_LENGTH,					\
  (nettle_armor_init_func) name##_encode_init,			\
  (nettle_armor_length_func) name##_encode_length,		\
  (nettle_armor_encode_update_func) name##_encode_update,	\
  (nettle_armor_encode_final_func) name##_encode_final,		\
  (nettle_armor_init_func) name##_decode_init,			\
  (nettle_armor_length_func) name##_decode_length,		\
  (nettle_armor_decode_update_func) name##_decode_update,	\
  (nettle_armor_decode_final_func) name##_decode_final,		\
}


extern const struct nettle_armor nettle_base64;
extern const struct nettle_armor nettle_base16;

#ifdef __cplusplus
}
#endif

#endif /* NETTLE_META_H_INCLUDED */
