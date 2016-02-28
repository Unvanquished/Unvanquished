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
#include "common/Common.h"
#include "qcommon/qcommon.h"
#include "CryptoChallenge.h"

static Cvar::Range<Cvar::Cvar<int>> cvar_server_challenge_timeout(
	"server.challenge.timeout",
	"Timeout (in seconds) of a challenge",
	Cvar::NONE,
	5,
	1,
	std::numeric_limits<int>::max()
);

static Cvar::Range<Cvar::Cvar<std::size_t>> cvar_server_challenge_length(
	"server.challenge.length",
	"Length in bytes of the challenge data (The hexadecimal representation will be twice as long)",
	Cvar::NONE,
	8,
	1,
	Crypto::Data().max_size()
);

static Cvar::Range<Cvar::Cvar<std::size_t>> cvar_server_challenge_count(
	"server.challenge.count",
	"Maximum number of active challenges kept in memory",
	Cvar::NONE,
	1024,
	1,
	std::deque<Challenge>().max_size()
);

Challenge::Duration Challenge::Timeout()
{
	return std::chrono::duration_cast<Duration>( std::chrono::seconds(
		cvar_server_challenge_timeout.Get()
	) );
}

std::size_t Challenge::Bytes()
{
	return cvar_server_challenge_length.Get();
}


std::string Challenge::GenerateString()
{
	Crypto::Data data( Bytes() );
	Sys::GenRandomBytes(data.data(), data.size());
	return Crypto::String(Crypto::Encoding::HexEncode(data));
}

bool Challenge::ValidString(const std::string& challenge)
{
	if ( challenge.empty() || challenge.size() % 2 )
	{
		return false;
	}

	return std::all_of(challenge.begin(), challenge.end(), Str::cisxdigit);
}

static std::deque<Challenge> challenges;

/*
 * Removes outdated challenges
 * PRE: The caller has acquired a lock on mutex
 */
static void Cleanup()
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

std::size_t ChallengeManager::MaxChallenges()
{
	return cvar_server_challenge_count.Get();
}

std::string ChallengeManager::GenerateChallenge( const netadr_t& source )
{
	auto challenge = Challenge( source );
	Push( challenge );
	return challenge.String();
}

void ChallengeManager::Push( const Challenge& challenge )
{
	Cleanup();

	if ( challenges.size() >= MaxChallenges() )
	{
		challenges.pop_front();
	}

	challenges.push_back( challenge );
}

bool ChallengeManager::Match( const Challenge& challenge, Challenge::Duration* ping )
{
	Cleanup();

	auto it = std::find_if( challenges.begin(), challenges.end(),
		[&challenge]( const Challenge& ch ) {
			return ch.Matches( challenge );
	} );

	if ( it != challenges.end() )
	{
		if ( ping )
		{
			*ping = it->Lifetime();
		}
		challenges.erase( it );
		return true;
	}

	return false;
}

void ChallengeManager::Clear()
{
	challenges.clear();
}

bool ChallengeManager::MatchString( const netadr_t& source,
									const std::string& challenge,
									Challenge::Duration* ping )
{
	auto challenge_data = Crypto::String(challenge);
	if ( Crypto::Encoding::HexDecode(challenge_data, challenge_data) )
	{
		return Match({source, challenge_data}, ping);
	}

	return false;
}
