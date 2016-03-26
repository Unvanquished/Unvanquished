/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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
#include "engine/qcommon/qcommon.h"
#include "Rcon.h"
#include "Crypto.h"
#include "framework/Network.h"
#include "engine/server/CryptoChallenge.h"

namespace Rcon {


Message::Message( const netadr_t& remote, std::string command,
				  Secure secure, std::string password, std::string challenge )
	: secure(secure),
	  challenge(std::move(challenge)),
	  command(std::move(command)),
	  password(std::move(password)),
	  remote(remote)
{}

Message::Message( std::string error_message )
	: error(std::move(error_message))
{}


bool Message::Valid(std::string *invalid_reason) const
{
	auto invalid = [invalid_reason](const char* reason)
	{
		if ( invalid_reason )
			*invalid_reason = reason;
		return false;
	};

	if ( !error.empty() )
	{
		return invalid(error.c_str());
	}

	if ( secure < Secure::Unencrypted || secure > Secure::EncryptedChallenge )
	{
		return invalid("Unknown secure protocol");
	}

	if ( password.empty() )
	{
		return invalid("Missing password");
	}

	if ( command.empty() )
	{
		return invalid("Missing command");
	}

	if ( secure == Secure::EncryptedChallenge && challenge.empty() )
	{
		return invalid("Missing challenge");
	}

	return true;
}

} // namespace Rcon
