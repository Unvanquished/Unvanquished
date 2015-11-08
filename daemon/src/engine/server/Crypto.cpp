/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2015, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "common/System.h"
#include "common/String.h"
#include "Crypto.h"
#include <nettle/aes.h>
#include <nettle/base64.h>
#include <nettle/md5.h>
#include <nettle/sha2.h>

// Compatibility with old nettle versions
#if !defined(AES256_KEY_SIZE)

#define AES256_KEY_SIZE 32

typedef aes_ctx compat_aes256_ctx;

static void nettle_compat_aes256_set_encrypt_key(compat_aes256_ctx *ctx, const uint8_t *key)
{
	nettle_aes_set_encrypt_key(ctx, AES256_KEY_SIZE, key);
}

static void nettle_compat_aes256_set_decrypt_key(compat_aes256_ctx *ctx, const uint8_t *key)
{
	nettle_aes_set_decrypt_key(ctx, AES256_KEY_SIZE, key);
}

static void nettle_compat_aes256_encrypt(const compat_aes256_ctx *ctx,
						   size_t length, uint8_t *dst,
						   const uint8_t *src)
{
	nettle_aes_encrypt(ctx, length, dst, src);
}

static void nettle_compat_aes256_decrypt(const compat_aes256_ctx *ctx,
						   size_t length, uint8_t *dst,
						   const uint8_t *src)
{
	nettle_aes_decrypt(ctx, length, dst, src);
}


static int nettle_compat_base64_decode_update(base64_decode_ctx *ctx,
		     size_t *dst_length,
		     uint8_t *dst,
		     size_t src_length,
		     const uint8_t *src)
{
	unsigned dst_length_uns = *dst_length;
	return nettle_base64_decode_update(ctx, &dst_length_uns, dst, src_length, src);
}

#undef aes256_set_encrypt_key
#define nettle_aes256_set_encrypt_key nettle_compat_aes256_set_encrypt_key
#define nettle_aes256_set_decrypt_key nettle_compat_aes256_set_decrypt_key
#define nettle_aes256_invert_key      nettle_compat_aes256_invert_key
#define nettle_aes256_encrypt         nettle_compat_aes256_encrypt
#define nettle_aes256_decrypt         nettle_compat_aes256_decrypt
#define aes256_ctx compat_aes256_ctx
#define nettle_base64_decode_update nettle_compat_base64_decode_update

#endif // NETTLE_VERSION_MAJOR < 3

namespace Crypto {

Data RandomData( std::size_t bytes )
{
    Data data( bytes );
    Sys::GenRandomBytes(data.data(), data.size());
    return data;
}

namespace Encoding {

/*
 * Translates binary data into a hexadecimal string
 */
Data HexEncode( const Data& input )
{
    std::ostringstream stream;
    stream.setf(std::ios::hex, std::ios::basefield);
    stream.fill('0');
    for ( auto ch : input )
    {
        stream.width(2);
        stream << int( ch );
    }
    auto string = stream.str();
    return Data(string.begin(), string.end());
}

/*
 * Translates a hexadecimal string into binary data
 * PRE: input is a valid hexadecimal string
 * POST: output contains the decoded string
 * Returns true on success
 * Note: If the decoding fails, output is unchanged
 */
bool HexDecode( const Data& input, Data& output )
{
    if ( input.size() % 2 )
    {
		return false;
    }

    Data mid( input.size() / 2 );

    for ( std::size_t i = 0; i < input.size(); i += 2 )
    {
        if ( !Str::cisxdigit( input[i] ) || !Str::cisxdigit( input[i+1] ) )
        {
            return false;
        }

        mid[ i / 2 ] = ( Str::GetHex( input[i] ) << 4 ) | Str::GetHex( input[i+1] );
    }

    output = mid;
    return true;
}

/*
 * Translates binary data into a base64 encoded string
 */
Data Base64Encode( const Data& input )
{
    base64_encode_ctx ctx;
    nettle_base64_encode_init( &ctx );

    Data output( BASE64_ENCODE_LENGTH( input.size() ) + BASE64_ENCODE_FINAL_LENGTH );
    int encoded_bytes = nettle_base64_encode_update(
        &ctx, output.data(), input.size(), input.data()
    );
    encoded_bytes += nettle_base64_encode_final( &ctx, output.data() + encoded_bytes );
    output.erase( output.begin() + encoded_bytes, output.end() );
    return output;
}

/*
 * Translates a base64 encoded string into binary data
 * PRE: input is a valid Base64 string
 * POST: output contains the decoded string
 * Returns true on success
 * Note: If the decoding fails, output is unchanged
 */
bool Base64Decode( const Data& input, Data& output )
{
    base64_decode_ctx ctx;
    nettle_base64_decode_init( &ctx );

    Data temp( BASE64_DECODE_LENGTH( input.size() ) );
    std::size_t decoded_bytes = 0;
    if ( !nettle_base64_decode_update( &ctx, &decoded_bytes, temp.data(),
                                       input.size(), input.data() ) )
    {
        return false;
    }

    if ( !nettle_base64_decode_final( &ctx ) )
    {
        return false;
    }

    temp.erase( temp.begin() + decoded_bytes, temp.end() );
    output = temp;
    return true;
}


} // namespace Encoding


namespace Hash {

Data Sha256( const Data& input )
{
    sha256_ctx ctx;
    nettle_sha256_init( &ctx );
    nettle_sha256_update( &ctx, input.size(), input.data() );
    Data output( SHA256_DIGEST_SIZE  );
    nettle_sha256_digest( &ctx, SHA256_DIGEST_SIZE, output.data() );
    return output;
}

Data Md5( const Data& input )
{
    md5_ctx ctx;
    nettle_md5_init( &ctx );
    nettle_md5_update( &ctx, input.size(), input.data() );
    Data output( MD5_DIGEST_SIZE );
    nettle_md5_digest( &ctx, MD5_DIGEST_SIZE, output.data() );
    return output;
}


} // namespace Hash

/*
 * Adds PKCS#7 padding to the data
 */
void AddPadding( Data& target, std::size_t block_size )
{
    auto pad = block_size - target.size() % block_size;
    target.resize( target.size() + pad, pad );
}

/*
 * Encrypts using the AES256 algorthim
 * PRE: Key is 256-bit (32 octects) long
 * POST: output contains the encrypted data
 * Notes: plain_text will be padded using the PKCS#7 algorthm,
 *        if the decoding fails, output is unchanged
 * Returns true on success
 */
bool Aes256Encrypt( Data plain_text, const Data& key, Data& output )
{
    if ( key.size() != AES256_KEY_SIZE )
    {
        return false;
    }
    aes256_ctx ctx;
    nettle_aes256_set_encrypt_key( &ctx, key.data() );
    AddPadding( plain_text, AES_BLOCK_SIZE );
    output.resize( plain_text.size(), 0 );
    nettle_aes256_encrypt( &ctx, plain_text.size(),
        output.data(), plain_text.data() );
    nettle_aes256_decrypt( &ctx, plain_text.size(),
        plain_text.data(), output.data() );
    return true;
}

/*
 * Encrypts using the AES256 algorthim
 * PRE: Key is 256-bit (32 octects) long,
 *      cypher_text.size() is an integer multiple of the AES block size
 * POST: output contains the decrypted data
 * Note: If the decoding fails, output is unchanged
 * Returns true on success
 */
bool Aes256Decrypt( Data cypher_text, const Data& key, Data& output )
{
    if ( key.size() != AES256_KEY_SIZE || cypher_text.size() % AES_BLOCK_SIZE )
    {
        return true;
    }
    aes256_ctx ctx;
    nettle_aes256_set_decrypt_key( &ctx, key.data() );
    output.resize( cypher_text.size(), 0 );
    nettle_aes256_decrypt( &ctx, cypher_text.size(),
        output.data(), cypher_text.data() );
    return true;
}

} // namespace Crypto
