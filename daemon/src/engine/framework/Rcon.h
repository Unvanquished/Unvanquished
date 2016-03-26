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
#ifndef COMMON_RCON_H_
#define COMMON_RCON_H_

#include <string>
#include "common/Cvar.h"

namespace Rcon {

/*
 * Secure RCon algorithm
 */
enum class Secure {
	Unencrypted,        // Allow unencrypted rcon
	EncryptedPlain,     // Require encryption
	EncryptedChallenge, // Require encryption and challege check
	Invalid             // Invalid protocol
};

/*
 * RCon message sent from a client to a server
 */
class Message
{
public:
	Message(const netadr_t& remote, std::string command,
			Secure secure, std::string password, std::string challenge = {});

	explicit Message(std::string error_message);

	/*
	 * Checks whether the message is valid (fits its own constraints)
	 */
	bool Valid(std::string *invalid_reason = nullptr) const;

	Secure      secure;
	std::string challenge;
	std::string command;
	std::string password;
	netadr_t    remote;
	std::string error;
};

} // namespace Rcon
#endif // COMMON_RCON_H_
