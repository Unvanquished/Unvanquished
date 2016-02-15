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

#ifndef COMMON_IPC_COMMON_H_
#define COMMON_IPC_COMMON_H_

/*
 * Contains definitions that are common to all IPC methods so that they are consistent
 */

namespace IPC {

    /*
     * IPC is everything that is related to the communication between the engine and
     * the VMs: in NaCl they are in different processes which means that communication
     * has to be Inter-Process Communication, via sockets and shared memory.
     * The most basic level of communication goes through a socket and is somewhat
     * expensive (10us overhead for a synchronous message, 1us for an asnc one) but
     * it has a strong ordering of messages and is the only way to wait one another
     * process: spinlocks in shared memory would prevent the OS from rescheduling etc.
     * For larger but very asynchronous messages, there is a command buffer in shared
     * memory that is more efficient but doesn't give a guarantee on when the messages
     * will be received.
     */

    /*
     * IPC descriptor which can be sent over a socket. You should treat this as an
     * opaque type and not access any of the fields directly.
     */
	struct FileDesc {
		Sys::OSHandle handle;
		#ifndef __native_client__
		int type;
		union {
			uint64_t size;
			int32_t flags;
		};
		#endif
        void Close() const;
	};

    /*
     * The messages sent between the VM and the engine are defined by a numerical
     * ID used for dispatch, a list of input types and an optional list of return
     * types. The ID is on 32 bits with usually 16 bits for coarse dispatch (at the
     * services level) and 16 bits for fine dispatch (function level).
     */

	// Special message ID used to indicate an RPC return and VM exit
	const uint32_t ID_RETURN = 0xffffffff;
	const uint32_t ID_EXIT = 0xfffffffe;

    // Combine a major and minor ID into a single number.
    // TODO we use a template, because we need the ID to be part of template
    // arguments and some compilers do not support constexpr yet.
	template<uint16_t Major, uint16_t Minor> struct Id {
		enum {
			value = (Major << 16) + Minor
		};
	};

    /*
     * Asynchronous message which does not wait for a reply, the argument types
     * can be any serializable type, you can declare a message type like so:
     *
     *     using MyMethodMsg = IPC::Message<IPC::Id<MY_MODULE, MY_METHOD>, std::string, int, bool>;
     */
	template<typename Id, typename... T> struct Message {
		enum {
			id = Id::value
		};
		using Inputs = std::tuple<T...>;
	};

	// Reply class which should only be used for the second parameter of SyncMessage
	template<typename... T> struct Reply {
		using Outputs = std::tuple<T...>;
	};

    /*
     * Synchronous message which waits for a reply. The reply can contain data. Both
     * the input and output types can be any serializable type, you can declare a
     * synchronous message type like so:
     *
     *     using MyMethodMsg = IPC::SyncMessage<
     *         Message<IPC::Id<MY_MODULE, MY_METHOD>, std::string, int, bool>,
     *         IPC::Reply<std::string>
     *     >;
     *
     * You can skip the Reply part if there is no return value, this is useful for
     * messages whose invocation will contain other sync messages, such as most
     * Engine -> VM messages.
     */
	template<typename Msg, typename Reply = Reply<>> struct SyncMessage {
		enum {
			id = Msg::id
		};
		using Inputs = typename Msg::Inputs;
		using Outputs = typename Reply::Outputs;
	};

} // namespace IPC

#endif // COMMON_IPC_IPC_H_
