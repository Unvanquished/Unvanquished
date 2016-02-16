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

#include "framework/Crypto.h"

class Challenge
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    /*
     * Time duration a challenge will be valid for
     */
    static Duration Timeout();

    /*
     * Size of the raw challenge data
     */
    static std::size_t Bytes();

    Challenge( const netadr_t& source, const Crypto::Data& challenge )
        : created( Clock::now() ),
          timeout( Timeout() ),
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
        return created + timeout;
    }

    /*
     * Whether the challenge is valid at the given time point
     */
    bool ValidAt( const TimePoint& time ) const
    {
        return created + timeout >= time;
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

    /*
     * Time when this challenge has been created
     */
    TimePoint Created() const
    {
        return created;
    }

    /*
     * Duration this challenge has been alive for
     */
    Duration Lifetime() const
    {
        return Clock::now() - created;
    }

private:
    /*
     * Generates a random challenge
     */
    std::string GenerateString();

    TimePoint    created;
	Duration     timeout;
    Crypto::Data challenge;
    netadr_t     source;

};

class ChallengeManager
{
public:
    static ChallengeManager& Get()
    {
        static ChallengeManager singleton;
        return singleton;
    }

    std::size_t MaxChallenges() const;

    /*
     * Generates a challenge for the given address and returns the challenge string
     */
    std::string GenerateChallenge( const netadr_t& source );

    /*
     * Add a challenge to the pool
     */
    void Push( const Challenge& challenge );

    /*
     * Check if a challenge matches any of the registered ones
     * If it does and it's valid, it'll return true and remove it
     *
     * If ping is not null, it will be set to the challenge ping time
     */
    bool Match( const Challenge& challenge, Challenge::Duration* ping = nullptr );


    /*
     * Removes all active challenges
     */
    void Clear();

    /*
     * Matches a challenge, the challenge string should be equal to what
     * GenerateChallenge() returned for the same source.
     *
     * If ping is not null, it will be set to the challenge ping time
     */
    bool MatchString( const netadr_t& source,
                      const std::string& challenge,
                      Challenge::Duration* ping = nullptr);

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
    void Cleanup();

    std::mutex mutex;
    std::deque<Challenge> challenges;
};

#endif // CRYPTOCHALLENGE_H
