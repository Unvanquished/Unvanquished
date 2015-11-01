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

class Data : public std::basic_string<uint8_t>
{
public:
    using basic_string::basic_string;

    explicit Data( std::size_t size )
        : basic_string( size, 0 )
    {}

    Data( const char* cstr )
        : basic_string( (const uint8_t*) cstr )
    {}

    Data( const std::string& str )
        : basic_string( str.begin(), str.end() )
    {}

    uint8_t* data()
    {
        return const_cast<uint8_t*>( basic_string::data() );
    }

    const uint8_t* data() const
    {
        return basic_string::data();
    }

    std::string str()
    {
        return std::string( begin(), end() );
    }
};

inline Data RandomData( std::size_t bytes )
{
    Data data( bytes );
    Sys::GenRandomBytes(data.data(), data.size());
    return data;
}


/*
 * Error thrown in case of an invalid operation
 */
class Error : public std::invalid_argument
{
    using std::invalid_argument::invalid_argument;
};


namespace Encoding {

/*
 * Translates binary data into a hexadecimal string
 */
inline std::string HexEncode( const Data& input )
{
    std::ostringstream stream;
    stream.setf(std::ios::hex, std::ios::basefield);
    stream.fill('0');
    for ( auto ch : input )
    {
        stream.width(2);
        stream << int( ch );
    }
    return stream.str();
}

/*
 * Translates a hexadecimal string into binary data
 * Throws Crypto::Error if the data is not a valid hexadecimal string
 */
inline Data HexDecode( const std::string& input )
{
    if ( input.size() % 2 )
    {
        throw Error( "Invalid Hex string" );
    }

    Data output( input.size() / 2 );

    for ( std::size_t i = 0; i < input.size(); i += 2 )
    {
        if ( !Str::cisxdigit( input[i] ) || !Str::cisxdigit( input[i+1] ) )
        {
            throw Error( "Invalid Hex string" );
        }

        output[ i / 2 ] = ( Str::gethex( input[i] ) << 4 ) | Str::gethex( input[i+1] );
    }

    return output;
}

/*
 * Translates binary data into a base64 encoded string
 */
inline std::string Base64Encode( const Data& input )
{
    base64_encode_ctx ctx;
    nettle_base64_encode_init( &ctx );

    Data output( BASE64_ENCODE_LENGTH( input.size() ) + BASE64_ENCODE_FINAL_LENGTH );
    int encoded_bytes = nettle_base64_encode_update(
        &ctx, output.data(), input.size(), input.data()
    );
    encoded_bytes += nettle_base64_encode_final( &ctx, output.data() + encoded_bytes );
    output.erase( encoded_bytes );
    return output.str();
}

/*
 * Translates a base64 encoded string into binary data
 * Throws Crypto::Error if the data is not a valid hexadecimal string
 */
inline Data Base64Decode( const Data& input )
{
    base64_decode_ctx ctx;
    nettle_base64_decode_init( &ctx );

    Data output( BASE64_DECODE_LENGTH( input.size() ) );
    std::size_t decoded_bytes = 0;
    if ( !nettle_base64_decode_update( &ctx, &decoded_bytes, output.data(),
                                       input.size(), input.data() ) )
    {
        throw Error( "Invalid Base64 string" );
    }

    if ( !nettle_base64_decode_final( &ctx ) )
    {
        throw Error( "Invalid Base64 string" );
    }

    output.erase( decoded_bytes );
    return output;
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

namespace Encryption {

/*
 * Adds PKCS#7 padding to the data
 */
inline void AddPadding( Data& target, std::size_t block_size = 8 )
{
    auto pad = block_size - target.size() % block_size;
    target.resize( target.size() + pad, pad );
}

/*
 * Base class for symmetric key encryption algorithms
 */
class Algorithm
{
public:

    virtual ~Algorithm(){}

    /*
     * Encrypt from cleartext to cypthertext using the given key
     * Might throw Error if the encryption fails
     */
    virtual Data Encrypt( Data plain_text, const Data& key ) const = 0;

    /*
     * Decrypt from cypthertext to cleartext using the given key
     * Might throw Error if the decryption fails
     */
    virtual Data Decrypt( Data cypher_text, const Data& key ) const = 0;

    /*
     * Generates a suitable key to use for encryption
     */
    virtual Data GenerateKey( const std::string& passphrase ) const = 0;

    /*
     * Name of the algorithm
     */
    virtual std::string Name() const = 0;

    /*
     * Size of a generated key, in bytes
     */
    virtual std::size_t KeySize() const = 0;

protected:
    /*
     * Checks whether a key is suitable to be used for encryption
     * Throws Error if that is not the case
     */
    virtual void ValidateKey( const Data& key ) const
    {
        if ( key.size() != KeySize() )
        {
            throw Error( "Invalid key passed to " + Name() );
        }
    }
};

/*
 * AES with 256 bit keys. Keys are generated from the SHA256 of the password
 */
class Aes256 : public Algorithm
{
public:
    Data Encrypt( Data plain_text, const Data& key ) const override
    {
        ValidateKey( key );
        aes256_ctx ctx;
        nettle_aes256_set_encrypt_key( &ctx, key.data() );
        AddPadding( plain_text, AES_BLOCK_SIZE );
        Data output( plain_text.size(), 0 );
        nettle_aes256_encrypt( &ctx, plain_text.length(),
            output.data(), plain_text.data() );
        nettle_aes256_decrypt( &ctx, plain_text.length(),
            plain_text.data(), output.data() );
        return output;
    }

    Data Decrypt( Data cypher_text, const Data& key ) const override
    {
        ValidateKey( key );
        if ( cypher_text.size() % AES_BLOCK_SIZE )
        {
            throw Error( "Invalid cyphertext size for " + Name() );
        }
        aes256_ctx ctx;
        nettle_aes256_set_decrypt_key( &ctx, key.data() );
        Data output( cypher_text.size(), 0 );
        nettle_aes256_decrypt( &ctx, cypher_text.length(),
            output.data(), cypher_text.data() );
        return output;
    }

    Data GenerateKey( const std::string& passphrase ) const
    {
        return Hash::Sha256( passphrase );
    }

    static std::string StaticName()
    {
        return "AES256";
    }

    std::string Name() const override
    {
        return StaticName();
    }

    std::size_t KeySize() const override
    {
        return 32;
    }
};

/*
 * Library of available algorithms
 */
class Library
{
public:
    static Library& Instance()
    {
        static Library singleton;
        return singleton;
    }

    /*
     * Registers an algorithm class
     * ALGORITHM must be derifed from Algorithm and have a static
     * public function called StaticName contextually convertible to std::string
     */
    template<class ALGORITHM>
        bool Register()
        {
            auto it = encryption_algorithms.find( ALGORITHM::StaticName() );
            if ( it != encryption_algorithms.end() )
            {
                return false;
            }
            encryption_algorithms[ ALGORITHM::StaticName() ].reset( new ALGORITHM );
            return true;
        }

    /*
     * Returns the algorithm with the given name or nullptr if it hasn't been
     * registered
     */
    Algorithm* EncryptionAlgorithm( const std::string& name )
    {
        auto it = encryption_algorithms.find( name );
        if ( it != encryption_algorithms.end() )
        {
            return it->second.get();
        }

        return nullptr;
    }

private:
    Library()
    {
        Register<Aes256>();
    }
    Library( const Library& ) = delete;
    Library& operator=( const Library& ) = delete;

    std::map<std::string, std::unique_ptr<Algorithm>> encryption_algorithms;
};

/*
 * Object that uses a specific algorithm to encrypt some data
 */
class Encryptor
{
public:
    explicit Encryptor( Algorithm* wrapped, const std::string& passphrase )
        : wrapped( wrapped ),
          key( wrapped->GenerateKey( passphrase ) )
    {
        if ( !wrapped )
        {
            throw Error( "Invalid encryption algorithm" );
        }
    }

    explicit Encryptor( const std::string& algo_name, const std::string& passphrase )
        : Encryptor(
            Library::Instance().EncryptionAlgorithm(algo_name),
            passphrase
        )
    {}

    Data Encrypt( const Data& plain_text ) const
    {
        return wrapped->Encrypt( plain_text, key );
    }

    Data Decrypt( const Data& plain_text ) const
    {
        return wrapped->Decrypt( plain_text, key );
    }

    const Data& Key() const
    {
        return key;
    }

    /*
     * Name of the algorithm
     */
    std::string AlgorithmName() const
    {
        return wrapped->Name();
    }

private:
    Algorithm* wrapped;
    Data key;
};

} // namespace Encryption

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
        return Crypto::Encoding::HexEncode( challenge );
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
    static ChallengeManager& Instance()
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
                    return challenge.ValidAt( now );
                }
            ),
            challenges.end()
        );
    }

    std::mutex mutex;
    std::deque<Challenge> challenges;
};

#endif // CRYPTOCHALLENGE_H
