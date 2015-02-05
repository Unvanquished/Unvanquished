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

#include "CommandBufferHost.h"

namespace IPC {

    CommandBufferHost::CommandBufferHost(std::string name): name(name), logs(name + ".commandBufferHost") {
    }

    void CommandBufferHost::Syscall(int index, Util::Reader& reader, IPC::Channel& channel) {
        switch (index) {
            case IPC::COMMAND_BUFFER_LOCATE:
                IPC::HandleMsg<IPC::CommandBufferLocateMsg>(channel, std::move(reader), [this] (IPC::SharedMemory mem0, IPC::SharedMemory mem1) {
                    this->Init(std::move(mem0), std::move(mem1));
                });
                break;

            case IPC::COMMAND_BUFFER_CONSUME:
                IPC::HandleMsg<IPC::CommandBufferConsumeMsg>(channel, std::move(reader), [this] (int i) {
                    this->Consume(i);
                });
                break;
        default:
            Sys::Drop("Bad CGame Command Buffer syscall minor number: %d", index);
        }
    }

    void CommandBufferHost::Init(IPC::SharedMemory mem0, IPC::SharedMemory mem1) {
        buffers[0].Init(std::move(mem0));
        buffers[1].Init(std::move(mem1));
        read[0] = read[1] = 0;

        logs.Debug("Received buffers of size %i and %i for %s", buffers[0].size, buffers[1].size, name);
    }

    void CommandBufferHost::Consume(int i) {
        if (i < 0 || i > 1) {
            Sys::Drop("Invalid command buffer to be consumed %d for %s", i, name);
        }

        logs.Debug("Consuming %i worth of data from buffer %i for %s", *buffers[i].writePos, i, name);
        while (read[i] != *buffers[i].writePos) {
            auto reader = ConsumeOne(i);

            uint32_t id = reader.Read<uint32_t>();

            int major = id >> 16;
            int minor = id & 0xffff;

            this->HandleCommandBufferSyscall(major, minor, reader);
        }

        read[i] = 0;
    }

    Util::Reader CommandBufferHost::ConsumeOne(int i) {
        size_t bufferSize = buffers[i].size;

        if (read[i] + sizeof(uint32_t) > bufferSize) {
            Sys::Drop("Command buffer overflow when reading size, for %s", name);
        }

        uint32_t length = *reinterpret_cast<uint32_t*>(buffers[i].data + read[i]);
        read[i] += sizeof(uint32_t);

        if (read[i] + length > bufferSize) {
            Sys::Drop("Command buffer overflow when reading data, for %s", name);
        }

        Util::Reader reader;
        auto& readerData = reader.GetData();
        readerData.resize(length);
        memcpy(readerData.data(), buffers[i].data + read[i], length);

        read[i] += length;

        return std::move(reader);
    }

} // namespace IPC
