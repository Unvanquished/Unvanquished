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

#ifndef SHARED_COMMAND_BUFFER_CLIENT_H_
#define SHARED_COMMAND_BUFFER_CLIENT_H_

#include "common/IPC/CommandBuffer.h"
#include "common/Serialize.h"
#include "VMMain.h"

namespace IPC {


    class CommandBufferClient {
        public:
            CommandBufferClient(std::string name);

            void Init();

            template<typename Message, typename... Args> void SendMsg(Args&&... args) {
                SendMsgImpl(Message(), std::forward<Args>(args)...);
            }

            template<typename Message, typename... Args> void SendMsgImpl(Message, Args&&... args) {
                static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value, "Incorrect number of arguments for CommandBufferClient::SendMsg");

                Util::Writer writer;
                writer.Write<uint32_t>(Message::id);
                writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);

                Write(writer);
            }

            void TryFlush();

        private:
            std::string name;
            Cvar::Range<Cvar::Cvar<int>> bufferSize;
            Log::Logger logs;
            IPC::CommandBuffer buffer;
            IPC::SharedMemory shm;
            bool initialized;

            void Write(Util::Writer& writer);

            bool CanWrite(size_t length);
            size_t RemainingSize();

            void Flush();
    };

}

#endif // SHARED_COMMAND_BUFFER_CLIENT_H_
