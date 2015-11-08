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

#include "Crypto.h"
#include "qcommon/qcommon.h"

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
