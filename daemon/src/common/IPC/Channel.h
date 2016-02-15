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

#ifndef COMMON_IPC_CHANNEL_H_
#define COMMON_IPC_CHANNEL_H_

#include "Primitives.h"

namespace IPC {

    /*
     * An IPC Channel wraps a socket between the engine and the VM and allows
     * sending synchronous messages and have several levels of function calls
     * between the engine and the VM.
     *
     * Additional functions extend the channel to send typed messages and wait
     * for a reply. While a message A is being processed by the other side of
     * the socket, here for the example the VM; the engine needs to handle
     * requests sent by the VM, so an additional message-handling function is
     * provided, this allows what look very much like "function calls" between
     * the engine and the VM. It can be recursive, but it is most probably a bad
     * idea to make recursive calls.
     *
     * To send a message to the other side of the socket, do the following:
     *
     *     IPC::SendMsg<MessageType>(channel, handlerFunction, message inputs..., message outputs...);
     *
     * In most cases the channel and handlerFunction will be the same and a
     * helper function will be defined so the call looks like the following:
     *
     *     something.SendMsg<MessageType>(message inputs..., message outputs...)
     *
     * In both cases the message outputs are taken by reference and will be
     * filled by the SendMsg. Likewise the inputs are taken by reference. The
     * message dispatching function must have the signature void(uint32_t ID, Util::Reader)
     *
     * To handle a message, once the method has been determined using the ID,
     * do the following, with reader the Reader given to the dispatch function:
     *
     *     IPC::HandleMsg<MssageType>(channel, std::move(reader), [&](int input1, std::string input2, int& output1, myStruct_t& output2) {
     *         // Handle the method call and fill both output references.
     *     });
     *
     * HandleMsg will unpack the content of the reader using the MessageType
     * provided and pass them by value (move semantics) to the lambda and at
     * the same time pass references to where the output should be written.
     * After the lambda has been called, it will serialize the outputs and
     * send it in the socket.
     */

    #ifdef BUILD_ENGINE
        static const bool TOPLEVEL_MSG_ALLOWED = true;
    #else
        static const bool TOPLEVEL_MSG_ALLOWED = false;
    #endif

    class Channel {
    public:
        Channel()
            : canSendSyncMsg(TOPLEVEL_MSG_ALLOWED), canSendAsyncMsg(TOPLEVEL_MSG_ALLOWED) {}
        Channel(Socket socket)
            : socket(std::move(socket)), canSendSyncMsg(TOPLEVEL_MSG_ALLOWED), canSendAsyncMsg(TOPLEVEL_MSG_ALLOWED) {}
        Channel(Channel&& other)
            : socket(std::move(other.socket)), canSendSyncMsg(TOPLEVEL_MSG_ALLOWED), canSendAsyncMsg(TOPLEVEL_MSG_ALLOWED) {}
        Channel& operator=(Channel&& other)
        {
            std::swap(socket, other.socket);
            canSendSyncMsg = other.canSendSyncMsg;
            canSendAsyncMsg = other.canSendAsyncMsg;
            return *this;
        }
        explicit operator bool() const
        {
            return bool(socket);
        }

        // Wrappers around socket functions
        void SendMsg(const Util::Writer& writer) const
        {
            socket.SendMsg(writer);
        }
        Util::Reader RecvMsg() const
        {
            return socket.RecvMsg();
        }
        void SetRecvTimeout(std::chrono::nanoseconds timeout)
        {
            socket.SetRecvTimeout(timeout);
        }

        // Wait for a synchronous message reply, returns the message ID and contents
        std::pair<uint32_t, Util::Reader> RecvReplyMsg()
        {
            Util::Reader reader = RecvMsg();

            uint32_t id = reader.Read<uint32_t>();
            return {id, std::move(reader)};
        }

    private:
        Socket socket;
        std::unordered_map<uint32_t, Util::Reader> replies;

    public:
        bool canSendSyncMsg;
        bool canSendAsyncMsg;
    };

    namespace detail {

        // Implementations of SendMsg for Message and SyncMessage
        template<typename Func, typename Id, typename... MsgArgs, typename... Args> void SendMsg(Channel& channel, Func&&, Message<Id, MsgArgs...>, Args&&... args)
        {
            using Message = Message<Id, MsgArgs...>;
            static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value, "Incorrect number of arguments for IPC::SendMsg");

            if (!channel.canSendAsyncMsg)
                Sys::Drop("Attempting to send a Message in VM toplevel with id: 0x%x", Message::id);

            Util::Writer writer;
            writer.Write<uint32_t>(Message::id);
            writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);
            channel.SendMsg(writer);
        }
        template<typename Func, typename Msg, typename Reply, typename... Args> void SendMsg(Channel& channel, Func&& messageHandler, SyncMessage<Msg, Reply>, Args&&... args)
        {
            using Message = SyncMessage<Msg, Reply>;
            static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value + std::tuple_size<typename Message::Outputs>::value, "Incorrect number of arguments for IPC::SendMsg");

            if (!channel.canSendSyncMsg)
                Sys::Drop("Attempting to send a SyncMessage while handling a Message or in VM toplevel with id: 0x%x", Message::id);

            Util::Writer writer;
            writer.Write<uint32_t>(Message::id);
            writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);
            channel.SendMsg(writer);

            while (true) {
                Util::Reader reader;
                uint32_t id;
                std::tie(id, reader) = channel.RecvReplyMsg();
                if (id == ID_RETURN) {
                    auto out = std::forward_as_tuple(std::forward<Args>(args)...);
                    reader.FillTuple<std::tuple_size<typename Message::Inputs>::value>(Util::TypeListFromTuple<typename Message::Outputs>(), out);
                    return;
                }
                messageHandler(id, std::move(reader));
            }
        }

        // Map a tuple to get the actual types returned by SerializeTraits::Read instead of the declared types
        template<typename T> struct MapTupleHelper {
            using type = decltype(Util::SerializeTraits<T>::Read(std::declval<Util::Reader&>()));
        };
        template<typename T> struct MapTuple {};
        template<typename... T> struct MapTuple<std::tuple<T...>> {
            using type = std::tuple<typename MapTupleHelper<T>::type...>;
        };

        // Implementations of HandleMsg for Message and SyncMessage
        template<typename Func, typename Id, typename... MsgArgs> void HandleMsg(Channel& channel, Message<Id, MsgArgs...>, Util::Reader reader, Func&& func)
        {
            using Message = Message<Id, MsgArgs...>;

            typename MapTuple<typename Message::Inputs>::type inputs;
            reader.FillTuple<0>(Util::TypeListFromTuple<typename Message::Inputs>(), inputs);
            reader.CheckEndRead();

            bool oldSync = channel.canSendSyncMsg;
            bool oldAsync = channel.canSendAsyncMsg;
            channel.canSendSyncMsg = false;
            channel.canSendAsyncMsg = true;
            Util::apply(std::forward<Func>(func), std::move(inputs));
            channel.canSendSyncMsg = oldSync;
            channel.canSendAsyncMsg = oldAsync;
        }
        template<typename Func, typename Msg, typename Reply> void HandleMsg(Channel& channel, SyncMessage<Msg, Reply>, Util::Reader reader, Func&& func)
        {
            using Message = SyncMessage<Msg, Reply>;

            typename MapTuple<typename Message::Inputs>::type inputs;
            typename Message::Outputs outputs;
            reader.FillTuple<0>(Util::TypeListFromTuple<typename Message::Inputs>(), inputs);
            reader.CheckEndRead();

            bool oldSync = channel.canSendSyncMsg;
            bool oldAsync = channel.canSendAsyncMsg;
            channel.canSendSyncMsg = true;
            channel.canSendAsyncMsg = true;
            Util::apply(std::forward<Func>(func), std::tuple_cat(Util::ref_tuple(std::move(inputs)), Util::ref_tuple(outputs)));
            channel.canSendSyncMsg = oldSync;
            channel.canSendAsyncMsg = oldAsync;

            Util::Writer writer;
            writer.Write<uint32_t>(ID_RETURN);
            writer.WriteTuple(Util::TypeListFromTuple<typename Message::Outputs>(), std::move(outputs));
            channel.SendMsg(writer);
        }

    } // namespace detail

    // Send a message to the given channel. If the message is synchronous then messageHandler is invoked for all
    // message that are recieved until ID_RETURN is recieved. Values returned by a synchronous message are
    // returned through reference parameters.
    template<typename Msg, typename Func, typename... Args> void SendMsg(Channel& channel, Func&& messageHandler, Args&&... args)
    {
        detail::SendMsg(channel, messageHandler, Msg(), std::forward<Args>(args)...);
    }

    // Handle an incoming message using a callback function (which can just be a lambda). If the message is
    // synchronous then outputs values are written to using reference parameters.
    template<typename Msg, typename Func> void HandleMsg(Channel& channel, Util::Reader reader, Func&& func)
    {
        detail::HandleMsg(channel, Msg(), std::move(reader), std::forward<Func>(func));
    }

} // namespace IPC

#endif // COMMON_IPC_CHANNEL_H_
