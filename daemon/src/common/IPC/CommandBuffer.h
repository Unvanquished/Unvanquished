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

#ifndef COMMON_IPC_COMMAND_BUFFER_H_
#define COMMON_IPC_COMMAND_BUFFER_H_

#include "CommonSyscalls.h"
#include "Primitives.h"

namespace IPC {

    /*
     * The command buffer is a single shared memory region, starting with a header
     * where signaling data is, followed by a big array that is treated as a circular
     * buffer. The signaling data contains the current offset of the reader and writer
     * of the command buffer.
     * This class contains the logic shared by the host and client of a command buffer
     * handling the fact that indices are in shared memory, circular calculations etc.
     */

    struct CommandBuffer {
        // Make sure the read and write offsets are in different cache lines to
        // avoid false sharing and a performance loss. 64 is because we expect and
        // know that the OS returns the shared memory pointer at memory page boundary
        // and that our structures all fit completely in one cache line.
        static const int READER_OFFSET = 0;
        static const int WRITER_OFFSET = 64;
        static const int DATA_OFFSET = 128;

        void Init(void* memory, size_t size);

        void Reset();

        // Synchronizes the reader data with the what is in shared memory
        // you need to call this to have the command buffer see the new
        // write offset for example. We do this to control when updates are
        // performed and simplify the logic.
        // Same thing for the reader data below.
        void LoadWriterData();
        void LoadReaderData();

        size_t GetMaxReadLength() const;
        size_t GetMaxWriteLength() const;

        bool CanRead(size_t len) const;
        bool CanWrite(size_t len) const;

        // Reads len bytes in out, starting at offset from the read pointer
        void Read(char* out, size_t len, size_t offset = 0);
        void Write(const char* in, size_t len, size_t offset = 0);

        // Advances the pointers and makes the update visible to the other end.
        // Make sure read advances correspond to write advances as the pointers
        // are re-aligned on advance.
        void AdvanceReadPointer(size_t offset);
        void AdvanceWritePointer(size_t offset);

        size_t GetSize() const;
    private:
        // Wraps offset in the circular buffer
        size_t Normalize(size_t offset) const;

        // Returns the position of offset if we consider the read (resp. write) offset to be the origin
        size_t FromRead(size_t offset) const;
        size_t FromWrite(size_t offset) const;

        void InternalRead(size_t fromOffset, char* out, size_t len);
        void InternalWrite(size_t toOffset, const char* in, size_t len);

        // If we have the read offset equal to the write offset then we don't know
        // if the full buffer is ready to be read or written. So we force a constant
        // "safety" offset between the read and the write pointer, here at 4 so we
        // start aligned. (otherwise it could be 1).
        static const int SAFETY_OFFSET = 4;

        // We assume these structure will be packed, and static assert on it.
        struct SharedWriterData {
            std::atomic<uint32_t> offset;
        };
        static_assert(offsetof(SharedWriterData, offset) == 0, "Wrong packing on SharedWriterData");

        struct SharedReaderData {
            std::atomic<uint32_t> offset;
        };
        static_assert(offsetof(SharedReaderData, offset) == 0, "Wrong packing on SharedReaderData");

        char* base;
        size_t writerOffset;
        size_t readerOffset;
        size_t size;
    };

    enum {
        COMMAND_BUFFER_LOCATE,
        COMMAND_BUFFER_CONSUME,
    };

    using CommandBufferLocateMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::COMMAND_BUFFER, COMMAND_BUFFER_LOCATE>, IPC::SharedMemory>
    >;

    using CommandBufferConsumeMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::COMMAND_BUFFER, COMMAND_BUFFER_CONSUME>>
    >;

} // namespace IPC

#endif // COMMON_IPC_COMMAND_BUFFER_H_
