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

    struct CommandBuffer {
        // We assume these structure will be packed, and static assert on it.
        // TODO IIRC the standard says we should still be using atomics
        // Also the current code might not work correctly, on arm as writes' order
        // might not be preserved by default, or if a compiler reorders them.
        struct WriterData {
            uint32_t offset;
        };
        static_assert(offsetof(WriterData, offset) == 0, "Wrong packing on WriterData");

        struct ReaderData {
            uint32_t offset;
        };
        static_assert(offsetof(ReaderData, offset) == 0, "Wrong packing on ReaderData");

        char* data;
        WriterData* sharedWriterData;
        ReaderData* sharedReaderData;
        WriterData writerData;
        ReaderData readerData;
        size_t size;

        // Make sure the read and write offsets are in different cache lines to
        // avoid false sharing and a performance loss. 128 is generous but it
        // with a cache line size of 64 bytes it make sure variable do not overlap
        // if the pointer is not a cache line boundary.
        static const int READER_OFFSET = 0;
        static const int WRITER_OFFSET = 128;
        static const int DATA_OFFSET = 256;

        void Init(void* memory, size_t size) {
            //TODO assert size at least DATA_OFFSET
            this->size = size - DATA_OFFSET;
            char* base = reinterpret_cast<char*>(memory);
            sharedWriterData = reinterpret_cast<WriterData*>(base + WRITER_OFFSET);
            sharedReaderData = reinterpret_cast<ReaderData*>(base + READER_OFFSET);
            data = reinterpret_cast<char*>(base + DATA_OFFSET);
        }

        void Reset() {
            sharedWriterData->offset = 0;
            sharedReaderData->offset = 0;
        }

        void LoadWriterData() {
            //TODO Drop on invalid offset
            writerData = *sharedWriterData;
        }

        void LoadReaderData() {
            //TODO Drop on invalid offset
            readerData = *sharedReaderData;
        }

        size_t FromRead(size_t offset) {
            if (offset < readerData.offset) {
                offset += size;
            }
            return offset - readerData.offset;
        }

        size_t FromWrite(size_t offset) {
            if (offset < writerData.offset) {
                offset += size;
            }
            return offset - writerData.offset;
        }

        size_t GetMaxReadLength() {
            return FromRead(writerData.offset);
        }

        size_t GetMaxWriteLength() {
            return FromWrite(readerData.offset);
        }

        bool CanRead(size_t len) {
            return FromRead(writerData.offset) >= len;
        }

        void Read(char* out, size_t len) {
            size_t canRead = size - readerData.offset;
            if (len >= canRead) {
                memcpy(out, data + readerData.offset, canRead);
                len -= canRead;
                out += canRead;
            }
            memcpy(out, data, len);
        }

        void AdvanceReadPointer(size_t offset) {
            // TODO assert that offset is < size
            size_t newOffset = readerData.offset + offset;
            if (offset >= size) {
                offset -= size;
            }
            readerData.offset = newOffset;
            sharedReaderData->offset = newOffset;
        }

        bool CanWrite(size_t len) {
            return FromWrite(readerData.offset) >= len;
        }

        void Write(const char* in, size_t len) {
            size_t canWrite = size - writerData.offset;
            if (len >= canWrite) {
                memcpy(data + writerData.offset, in, canWrite);
                len -= canWrite;
                in += canWrite;
            }
            memcpy(data, in, len);
        }

        void AdvanceWritePointer(size_t offset) {
            // TODO assert that offset is < size
            size_t newOffset = writerData.offset + offset;
            if (offset >= size) {
                offset -= size;
            }
            writerData.offset = newOffset;
            sharedWriterData->offset = newOffset;
        }

    };

    enum {
        COMMAND_BUFFER_LOCATE,
        COMMAND_BUFFER_CONSUME,
    };

    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::COMMAND_BUFFER, COMMAND_BUFFER_LOCATE>, IPC::SharedMemory>
    > CommandBufferLocateMsg;

    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::COMMAND_BUFFER, COMMAND_BUFFER_CONSUME>>
    > CommandBufferConsumeMsg;

} // namespace IPC

#endif // COMMON_IPC_COMMAND_BUFFER_H_
