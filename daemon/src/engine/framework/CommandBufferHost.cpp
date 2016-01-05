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

#include "CommandBufferHost.h"

namespace IPC {

    CommandBufferHost::CommandBufferHost(std::string name): name(name), logs(name + ".commandBufferHost") {
    }

    void CommandBufferHost::Syscall(int index, Util::Reader& reader, IPC::Channel& channel) {
        switch (index) {
            case IPC::COMMAND_BUFFER_LOCATE:
                IPC::HandleMsg<IPC::CommandBufferLocateMsg>(channel, std::move(reader), [this] (IPC::SharedMemory mem) {
                    this->Init(std::move(mem));
                });
                break;

            case IPC::COMMAND_BUFFER_CONSUME:
                IPC::HandleMsg<IPC::CommandBufferConsumeMsg>(channel, std::move(reader), [this] () {
                    this->Consume();
                });
                break;
        default:
            Sys::Drop("Bad CGame Command Buffer syscall minor number: %d", index);
        }
    }

    void CommandBufferHost::Init(IPC::SharedMemory mem) {
        shm = std::move(mem);
        buffer.Init(shm.GetBase(), shm.GetSize());

        logs.Debug("Received buffers of size %i for %s", buffer.GetSize(), name);
    }

    void CommandBufferHost::Consume() {
        buffer.LoadWriterData();
        logs.Debug("Consuming up to %i data from buffer for %s", buffer.GetMaxReadLength(), name);
        bool consuming = true;
        //TODO set fixed bound too

        while(consuming) {
            Util::Reader reader;
            consuming = ConsumeOne(reader);

            if (consuming) {
                uint32_t id = reader.Read<uint32_t>();
                int major = id >> 16;
                int minor = id & 0xffff;
                this->HandleCommandBufferSyscall(major, minor, reader);
            }
            //TODO add more logic to stop consuming (e.g. when the socket is ready)
        }
    }

    bool CommandBufferHost::ConsumeOne(Util::Reader& reader) {
        if (!buffer.CanRead(sizeof(uint32_t))) {
            buffer.LoadWriterData();
            if (!buffer.CanRead(sizeof(uint32_t))) {
                if (buffer.GetMaxReadLength() != 0) {
                    Sys::Drop("Command buffer for %s had an incomplete length write", name);
                }
                return false;
            }
        }
        uint32_t size;
        buffer.Read((char*)&size, sizeof(uint32_t));

        if (!buffer.CanRead(size + sizeof(uint32_t))) {
            Sys::Drop("Command buffer for %s had an incomplete message write", name);
            return false;
        }
        std::vector<char>& readerData = reader.GetData();
        readerData.resize(size);
        buffer.Read(readerData.data(), size, sizeof(uint32_t));

        buffer.AdvanceReadPointer(size + sizeof(uint32_t));

        return true;
    }

} // namespace IPC
