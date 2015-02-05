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

#ifndef COMMON_IPC_COMMAND_BUFFER_H_
#define COMMON_IPC_COMMAND_BUFFER_H_

#include "CommonSyscalls.h"
#include "Primitives.h"

namespace IPC {

    struct CommandBufferData {
        IPC::SharedMemory shm;
        char* data;
        uint32_t* writePos;
        uint32_t* readPos;
        size_t size;

        static const int READ_OFFSET = 0;
        static const int WRITE_OFFSET = 64;
        static const int DATA_OFFSET = 128;

        void Init(IPC::SharedMemory shmem) {
            shm = std::move(shmem);
            size = shm.GetSize() - DATA_OFFSET;
            char* base = reinterpret_cast<char*>(shm.GetBase());
            writePos = reinterpret_cast<uint32_t*>(base + WRITE_OFFSET);
            readPos = reinterpret_cast<uint32_t*>(base + READ_OFFSET);
            data = reinterpret_cast<char*>(base + DATA_OFFSET);
        }

        void Close() {
            shm.Close();
        }
    };

    enum {
        COMMAND_BUFFER_LOCATE,
        COMMAND_BUFFER_CONSUME,
    };

    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::COMMAND_BUFFER, COMMAND_BUFFER_LOCATE>, IPC::SharedMemory, IPC::SharedMemory>
    > CommandBufferLocateMsg;

    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::COMMAND_BUFFER, COMMAND_BUFFER_CONSUME>, int>
    > CommandBufferConsumeMsg;

} // namespace IPC

#endif // COMMON_IPC_COMMAND_BUFFER_H_
