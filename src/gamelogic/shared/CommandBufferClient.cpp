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

#include "CommandBufferClient.h"

namespace IPC {

    static const int DEFAULT_SIZE = 2 * 1024 * 1024;

    CommandBufferClient::CommandBufferClient(std::string name)
    : name(name),
    bufferSize("vm." + name + "commandBuffer.size", "The size of the shared memory command buffer used by " + name, Cvar::NONE, DEFAULT_SIZE, 100, 16 * 1024 * 1024),
    logs(name + ".commandBuffer") {
    }

    void CommandBufferClient::Init() {
        buffers[0].Init(IPC::SharedMemory::Create(bufferSize.Get()));
        buffers[1].Init(IPC::SharedMemory::Create(bufferSize.Get()));
        current = 0;
        written[0] = written[1] = 0;

        VM::SendMsg<CommandBufferLocateMsg>(buffers[0].shm, buffers[1].shm);

        logs.Debug("Created double buffers of size %i for %s", bufferSize.Get(), name);
    }

    void CommandBufferClient::TryFlush() {
        if (VM::rootChannel.handlingAsyncMsg) {
            Sys::Drop("Trying to Flush the %s command buffer when handling an async message", name);
        }
        if (written[current] == 0) {
            return;
        }
        Flush();
    }

    void CommandBufferClient::Write(Util::Writer& writer) {
        auto& writerData = writer.GetData();
        size_t dataSize = writerData.size();

        if (!CanWrite(dataSize)) {
            logs.Debug("Message of size %i for %s doesn't fit the remaining %i, flushing.", dataSize, name, RemainingSize());
            Flush();
            if (!CanWrite(dataSize)) {
                Sys::Drop("Message of size %i doesn't fit in buffer for %s of size %i", dataSize, name, buffers[current].size);
            }
        }

        if (writer.GetHandles().size() != 0) {
            Sys::Drop("Command buffer %s: handles sent to the command buffer", name);
        }

        char* data = buffers[current].data;

        *reinterpret_cast<uint32_t*>(data + written[current]) = writerData.size();
        memcpy(data + written[current] + sizeof(uint32_t), writerData.data(), writerData.size());
        written[current] += sizeof(uint32_t) + writerData.size();

        *buffers[current].writePos = written[current];
    }

    bool CommandBufferClient::CanWrite(size_t length) {
        return length + sizeof(uint32_t) <= RemainingSize();
    }

    size_t CommandBufferClient::RemainingSize() {
        return buffers[current].size - written[current];
    }

    void CommandBufferClient::Flush() {//TODO prevent recursion
        written[current] = 0;

        logs.Debug("Flushing %s buffer %i containing %i worth of message", name, current, written[current]);

        VM::SendMsg<CommandBufferConsumeMsg>(current);
        //current ^= 1;
    }

} // namespace IPC
