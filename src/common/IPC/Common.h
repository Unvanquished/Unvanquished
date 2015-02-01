/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2015, Daemon Developers
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

#ifndef COMMON_IPC_COMMON_H_
#define COMMON_IPC_COMMON_H_

namespace IPC {

	// IPC descriptor which can be sent over a socket. You should treat this as an
	// opaque type and not access any of the fields directly.
	struct Desc {
		Sys::OSHandle handle;
		#ifndef __native_client__
		int type;
		union {
			uint64_t size;
			int32_t flags;
		};
		#endif
	};
	void CloseDesc(const Desc& desc);

	// Message ID to indicate an RPC return
	const uint32_t ID_RETURN = 0xffffffff;

	// Combine a major and minor ID into a single number
	template<uint16_t Major, uint16_t Minor> struct Id {
		enum {
			value = (Major << 16) + Minor
		};
	};

	// Asynchronous message which does not wait for a reply
	template<typename Id, typename... T> struct Message {
		enum {
			id = Id::value
		};
		typedef std::tuple<T...> Inputs;
	};

	// Reply class which should only be used for the second parameter of SyncMessage
	template<typename... T> struct Reply {
		typedef std::tuple<T...> Outputs;
	};

	// Synchronous message which waits for a reply. The reply can contain data.
	template<typename Msg, typename Reply = Reply<>> struct SyncMessage {
		enum {
			id = Msg::id
		};
		typedef typename Msg::Inputs Inputs;
		typedef typename Reply::Outputs Outputs;
	};

} // namespace IPC

#endif // COMMON_IPC_IPC_H_
