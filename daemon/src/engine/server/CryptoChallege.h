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

#ifndef CRYPTOCHALLENGE_H
#define CRYPTOCHALLENGE_H

#include "common/System.h"
#include "engine/qcommon/qcommon.h"
#include <nettle/aes.h>
#include <nettle/base64.h>
#include <nettle/md5.h>
#include <nettle/sha2.h>
#include <nettle/pbkdf2.h>

namespace Crypto {

using Data = std::vector<uint8_t>;

inline Data RandomData( std::size_t bytes )
{
    Data data( bytes );
    Sys::GenRandomBytes(data.data(), data.size());
    return data;
}

inline Data String(const std::string& string)
{
    return Data(string.begin(), string.end());
}

inline std::string String(const Data string)
{
    return std::string(string.begin(), string.end());
}

namespace Encoding {

/*
 * Translates binary data into a hexadecimal string
 */
inline Data HexEncode( const Data& input )
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
inline bool HexDecode( const Data& input, Data& output )
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
inline Data Base64Encode( const Data& input )
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
inline bool Base64Decode( const Data& input, Data& output )
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

inline Data Sha256( const Data& input )
{
    sha256_ctx ctx;
    nettle_sha256_init( &ctx );
    nettle_sha256_update( &ctx, input.size(), input.data() );
    Data output( SHA256_DIGEST_SIZE  );
    nettle_sha256_digest( &ctx, SHA256_DIGEST_SIZE, output.data() );
    return output;
}

inline Data Md5( const Data& input )
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
inline void AddPadding( Data& target, std::size_t block_size = 8 )
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
inline bool Aes256Encrypt( Data plain_text, const Data& key, Data& output )
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
inline bool Aes256Decrypt( Data cypher_text, const Data& key, Data& output )
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

class Challenge
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    /*
     * Time duration a challenge will be valid for
     */
    static Duration Timeout()
    {
        // TODO Read this from a cvar
        return std::chrono::duration_cast<Duration>( std::chrono::seconds( 5 ) );
    }

    /*
     * Size of the raw challenge data
     */
    static std::size_t Bytes()
    {
        // TODO Read this from a cvar
        return 8;
    }

    Challenge( const netadr_t& source, const Crypto::Data& challenge )
        : valid_until( Clock::now() + Timeout() ),
          challenge( challenge ),
          source( source )
    {
        // The port number might change in connectionless commands
        this->source.port = this->source.port4 = this->source.port6 = 0;
    }

    explicit Challenge( const netadr_t& source )
        : Challenge( source, Crypto::RandomData( Bytes() ) )
    {
    }

    /*
     * Maximum time this challenge should be considered valid
     */
    TimePoint ValidUntil() const
    {
        return valid_until;
    }

    /*
     * Whether the challenge is valid at the given time point
     */
    bool ValidAt( const TimePoint& time ) const
    {
        return valid_until >= time;
    }

    /*
     * Whether two challenges match
     */
    bool Matches( const Challenge& other ) const
    {
        if ( challenge != other.challenge )
            return false;

        return NET_CompareAdr( source, other.source );
    }

    /*
     * Challenge as a hex string
     */
    std::string String() const
    {
        return Crypto::String( Crypto::Encoding::HexEncode( challenge ) );
    }

private:
    /*
     * Generates a random challenge
     */
    std::string GenerateString()
    {
        std::vector<uint8_t> data( Bytes() );
        Sys::GenRandomBytes(data.data(), data.size());
        std::ostringstream stream;
        stream.setf(std::ios::hex, std::ios::basefield);
        stream.fill('0');
        for ( auto ch : data )
        {
            stream.width(2);
            stream << int( ch );
        }
        return stream.str();
    }

    TimePoint valid_until;
    Crypto::Data challenge;
    netadr_t source;

};

class ChallengeManager
{
public:
    static ChallengeManager& Get()
    {
        static ChallengeManager singleton;
        return singleton;
    }

    int MaxChallenges() const
    {
        // TODO Read this from a cvar
        return 1024;
    }

    /*
     * Generates a challenge for the given address and returns the challenge string
     */
    std::string GenerateChallenge( const netadr_t& source )
    {
        auto challenge = Challenge( source );
        Push( challenge );
        return challenge.String();
    }

    /*
     * Add a challenge to the pool
     */
    void Push( const Challenge& challenge )
    {
        auto lock = Lock();

        Cleanup();

        if ( challenges.size() >= MaxChallenges() )
        {
            challenges.pop_front();
        }

        challenges.push_back( challenge );
    }

    /*
     * Check if a challenge matches any of the registered ones
     * If it does and it's valid, it'll return false
     */
    bool Match( const Challenge& challenge )
    {
        auto lock = Lock();

        Cleanup();

        auto it = std::find_if( challenges.begin(), challenges.end(),
            [&challenge]( const Challenge& ch ) {
                return ch.Matches( challenge );
        } );

        if ( it != challenges.end() )
        {
            challenges.erase( it );
            return true;
        }

        return false;
    }

private:
    ChallengeManager() = default;
    ChallengeManager( const ChallengeManager& ) = delete;
    ChallengeManager& operator=( const ChallengeManager& ) = delete;

    std::unique_lock<std::mutex> Lock()
    {
        return std::unique_lock<std::mutex>{mutex};
    }

    /*
     * Removes outdated challenges
     * PRE: The caller has acquired a lock on mutex
     */
    void Cleanup()
    {
        auto now = Challenge::Clock::now();
        challenges.erase(
            std::remove_if( challenges.begin(), challenges.end(),
                [&now]( const Challenge& challenge ) {
                    return !challenge.ValidAt( now );
                }
            ),
            challenges.end()
        );
    }

    std::mutex mutex;
    std::deque<Challenge> challenges;
};

#endif // CRYPTOCHALLENGE_H
