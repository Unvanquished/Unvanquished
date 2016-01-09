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

#include "CommandBufferClient.h"

namespace IPC {

    static const int DEFAULT_SIZE = 2 * 1024 * 1024;
    static const int MINIMUM_SIZE = CommandBuffer::DATA_OFFSET + 128;

    CommandBufferClient::CommandBufferClient(std::string name)
    : name(name),
    bufferSize("vm." + name + "commandBuffer.size", "The size of the shared memory command buffer used by " + name, Cvar::NONE, DEFAULT_SIZE, MINIMUM_SIZE, 16 * 1024 * 1024),
    logs(name + ".commandBufferClient"), initialized(false) {
    }

    void CommandBufferClient::Init() {
        shm = IPC::SharedMemory::Create(bufferSize.Get());
        buffer.Init(shm.GetBase(), shm.GetSize());
        buffer.Reset();

        VM::SendMsg<CommandBufferLocateMsg>(shm);

        initialized = true;
        logs.Debug("Created circular buffer of size %i for %s", bufferSize.Get(), name);
    }

    void CommandBufferClient::TryFlush() {
        if (!initialized) {
            return;
        }

        if (!VM::rootChannel.canSendSyncMsg) {
            Sys::Drop("Trying to Flush the %s command buffer when handling an async message or in toplevel", name);
        }

        buffer.LoadReaderData();
        if (buffer.GetMaxReadLength() == 0) {
            return;
        }
        Flush();
    }

    void CommandBufferClient::Write(Util::Writer& writer) {
        if (!VM::rootChannel.canSendSyncMsg) {
            Sys::Drop("Trying to write to the %s command buffer when handling an async message or in toplevel", name);
        }
        auto& writerData = writer.GetData();
        uint32_t dataSize = writerData.size();
        uint32_t totalSize = dataSize + sizeof(uint32_t);

        if (writer.GetHandles().size() != 0) {
            Sys::Drop("Command buffer %s: handles sent to the command buffer", name);
        }

        buffer.LoadReaderData();
        if (!buffer.CanWrite(totalSize)) {
            logs.Debug("Message of size %i(+4) for %s doesn't fit the remaining %i, flushing.", dataSize, name, buffer.GetMaxWriteLength());
            Flush();
            buffer.LoadReaderData();
            if (!buffer.CanWrite(totalSize)) {
                Sys::Drop("Message of size %i(+4) doesn't fit in buffer for %s of size %i", dataSize, name, buffer.GetSize());
            }
        }

        buffer.Write((char*)&dataSize, sizeof(uint32_t));
        buffer.Write(writerData.data(), dataSize, sizeof(uint32_t));

        buffer.AdvanceWritePointer(totalSize);
    }

    void CommandBufferClient::Flush() {//TODO prevent recursion
        logs.Debug("Flushing %s command buffer with up to %i bytes", name, buffer.GetMaxReadLength());

        VM::SendMsg<CommandBufferConsumeMsg>();
    }

} // namespace IPC
