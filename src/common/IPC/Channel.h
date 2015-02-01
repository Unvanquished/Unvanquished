/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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

// An IPC channel wraps a socket and provides additional support for sending
// synchronous typed messages over it.
class Channel {
public:
	Channel()
		: counter(0), handlingAsyncMsg(false) {}
	Channel(Socket socket)
		: socket(std::move(socket)), counter(0), handlingAsyncMsg(false) {}
	Channel(Channel&& other)
		: socket(std::move(other.socket)), counter(0), handlingAsyncMsg(false) {}
	Channel& operator=(Channel&& other)
	{
		std::swap(socket, other.socket);
		counter = other.counter;
		handlingAsyncMsg = other.handlingAsyncMsg;
		return *this;
	}
	explicit operator bool() const
	{
		return bool(socket);
	}

	// Wrappers around socket functions
	void SendMsg(const Serialize::Writer& writer) const
	{
		socket.SendMsg(writer);
	}
    Serialize::Reader RecvMsg() const
	{
		return socket.RecvMsg();
	}
	void SetRecvTimeout(std::chrono::nanoseconds timeout)
	{
		socket.SetRecvTimeout(timeout);
	}

	// Generate a unique message key to match messages with replies
	uint32_t GenMsgKey()
	{
		return counter++;
	}

	// Wait for a synchronous message reply, returns the message ID and contents
	std::pair<uint32_t, Serialize::Reader> RecvReplyMsg(uint32_t key)
	{
		// If we have already recieved a reply for this key, just return that
		auto it = replies.find(key);
		if (it != replies.end()) {
            Serialize::Reader reader = std::move(it->second);
			replies.erase(it);
			return {ID_RETURN, std::move(reader)};
		}

		// Otherwise handle incoming messages until we find the reply we want
		while (true) {
            Serialize::Reader reader = RecvMsg();

			// Is this a reply?
			uint32_t id = reader.Read<uint32_t>();
			if (id != ID_RETURN)
				return {id, std::move(reader)};

			// Is this the reply we are expecting?
			uint32_t msg_key = reader.Read<uint32_t>();
			if (msg_key == key)
				return {ID_RETURN, std::move(reader)};

			// Save the reply for later
			replies.insert(std::make_pair(msg_key, std::move(reader)));
		}
	}

private:
	Socket socket;
	uint32_t counter;
	std::unordered_map<uint32_t, Serialize::Reader> replies;

public:
	bool handlingAsyncMsg;
};

namespace detail {

// Implementations of SendMsg for Message and SyncMessage
template<typename Func, typename Id, typename... MsgArgs, typename... Args> void SendMsg(Channel& channel, Func&&, Message<Id, MsgArgs...>, Args&&... args)
{
	typedef Message<Id, MsgArgs...> Message;
	static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value, "Incorrect number of arguments for IPC::SendMsg");

    Serialize::Writer writer;
	writer.Write<uint32_t>(Message::id);
	writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);
	channel.SendMsg(writer);
}
template<typename Func, typename Msg, typename Reply, typename... Args> void SendMsg(Channel& channel, Func&& messageHandler, SyncMessage<Msg, Reply>, Args&&... args)
{
	typedef SyncMessage<Msg, Reply> Message;
	static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value + std::tuple_size<typename Message::Outputs>::value, "Incorrect number of arguments for IPC::SendMsg");

	if (channel.handlingAsyncMsg)
		Com_Error(ERR_DROP, "Attempting to send a SyncMessage while handling a Message");

    Serialize::Writer writer;
	writer.Write<uint32_t>(Message::id);
	uint32_t key = channel.GenMsgKey();
	writer.Write<uint32_t>(key);
	writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);
	channel.SendMsg(writer);

	while (true) {
        Serialize::Reader reader;
		uint32_t id;
		std::tie(id, reader) = channel.RecvReplyMsg(key);
		if (id == ID_RETURN) {
			auto out = std::forward_as_tuple(std::forward<Args>(args)...);
			reader.FillTuple<std::tuple_size<typename Message::Inputs>::value>(Util::TypeListFromTuple<typename Message::Outputs>(), out);
			return;
		}
		messageHandler(id, std::move(reader));
	}
}

// Implementations of HandleMsg for Message and SyncMessage
template<typename Func, typename Id, typename... MsgArgs> void HandleMsg(Channel& channel, Message<Id, MsgArgs...>, Serialize::Reader reader, Func&& func)
{
	typedef Message<Id, MsgArgs...> Message;

	typename Serialize::Reader::MapTuple<typename Message::Inputs>::type inputs;
	reader.FillTuple<0>(Util::TypeListFromTuple<typename Message::Inputs>(), inputs);

	bool old = channel.handlingAsyncMsg;
	channel.handlingAsyncMsg = true;
	Util::apply(std::forward<Func>(func), std::move(inputs));
	channel.handlingAsyncMsg = old;
}
template<typename Func, typename Msg, typename Reply> void HandleMsg(Channel& channel, SyncMessage<Msg, Reply>, Serialize::Reader reader, Func&& func)
{
	typedef SyncMessage<Msg, Reply> Message;

	uint32_t key = reader.Read<uint32_t>();
	typename Serialize::Reader::MapTuple<typename Message::Inputs>::type inputs;
	typename Message::Outputs outputs;
	reader.FillTuple<0>(Util::TypeListFromTuple<typename Message::Inputs>(), inputs);
	Util::apply(std::forward<Func>(func), std::tuple_cat(Util::ref_tuple(std::move(inputs)), Util::ref_tuple(outputs)));

    Serialize::Writer writer;
	writer.Write<uint32_t>(ID_RETURN);
	writer.Write<uint32_t>(key);
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
template<typename Msg, typename Func> void HandleMsg(Channel& channel, Serialize::Reader reader, Func&& func)
{
	detail::HandleMsg(channel, Msg(), std::move(reader), std::forward<Func>(func));
}

} // namespace IPC

#endif // COMMON_IPC_CHANNEL_H_
