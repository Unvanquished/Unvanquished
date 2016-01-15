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

#ifndef COMMON_COMMON_SYSCALLS_H_
#define COMMON_COMMON_SYSCALLS_H_

#include "Primitives.h"
#include <common/FileSystem.h>

namespace VM {

    /*
     * This file contains the definition of the message types that are common
     * to all the VMs and that are implemented in
     *   - CommonVMServices in the engine
     *   - CommonProxies in the VM
     * QVM is a special message ID major number that will be used for all the
     * messages for the former QVM syscalls.
     * LAST_COMMON_SYSCALL is the first ID available for VM-specific use.
     */

    enum {
        QVM,
        QVM_COMMON,
        MISC,
        COMMAND,
        CVAR,
        LOG,
        FILESYSTEM,
        COMMAND_BUFFER,
        LAST_COMMON_SYSCALL
    };

    // Common QVM syscalls
    // TODO kill them or move them somewhere else?
    enum {
        QVM_COMMON_PRINT,
        QVM_COMMON_ERROR,
        QVM_COMMON_SEND_CONSOLE_COMMAND,

        QVM_COMMON_FS_FOPEN_FILE,
        QVM_COMMON_FS_READ,
        QVM_COMMON_FS_WRITE,
        QVM_COMMON_FS_SEEK,
        QVM_COMMON_FS_TELL,
		QVM_COMMON_FS_FILELENGTH,
        QVM_COMMON_FS_RENAME,
        QVM_COMMON_FS_FCLOSE_FILE,
        QVM_COMMON_FS_GET_FILE_LIST,
        QVM_COMMON_FS_GET_FILE_LIST_RECURSIVE,
        QVM_COMMON_FS_FIND_PAK,
        QVM_COMMON_FS_LOAD_PAK,
        QVM_COMMON_FS_LOAD_MAP_METADATA,

        QVM_COMMON_PARSE_ADD_GLOBAL_DEFINE,
        QVM_COMMON_PARSE_LOAD_SOURCE,
        QVM_COMMON_PARSE_FREE_SOURCE,
        QVM_COMMON_PARSE_READ_TOKEN,
        QVM_COMMON_PARSE_SOURCE_FILE_AND_LINE,
    };

    using PrintMsg = IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PRINT>, std::string>;
    using ErrorMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_ERROR>, std::string>
    >;
    // LogMsg TODO
    using SendConsoleCommandMsg = IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_SEND_CONSOLE_COMMAND>, std::string>;

    using FSFOpenFileMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FOPEN_FILE>, std::string, bool, int>,
        IPC::Reply<int, int>
    >;
    using FSReadMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_READ>, int, int>,
        IPC::Reply<std::string, int>
    >;
    using FSWriteMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_WRITE>, int, std::string>,
        IPC::Reply<int>
    >;
    using FSSeekMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_SEEK>, int, int, int>,
        IPC::Reply<int>
    >;
    using FSTellMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_TELL>, fileHandle_t>,
        IPC::Reply<int>
    >;
	using FSFileLengthMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FILELENGTH>, fileHandle_t>,
		IPC::Reply<int>
	>;
    using FSRenameMsg = IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_RENAME>, std::string, std::string>;
    using FSFCloseFileMsg = IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FCLOSE_FILE>, int>;
    using FSGetFileListMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_GET_FILE_LIST>, std::string, std::string, int>,
        IPC::Reply<int, std::string>
    >;
    using FSGetFileListRecursiveMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_GET_FILE_LIST_RECURSIVE>, std::string, std::string, int>,
        IPC::Reply<int, std::string>
    >;
    using FSFindPakMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FIND_PAK>, std::string>,
        IPC::Reply<bool>
    >;
    using FSLoadPakMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_LOAD_PAK>, std::string, std::string>,
        IPC::Reply<bool>
    >;
    using FSLoadMapMetadataMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_LOAD_MAP_METADATA>>
    >;

    using ParseAddGlobalDefineMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_ADD_GLOBAL_DEFINE>, std::string>,
        IPC::Reply<int>
    > ;
    using ParseLoadSourceMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_LOAD_SOURCE>, std::string>,
        IPC::Reply<int>
    >;
    using ParseFreeSourceMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_FREE_SOURCE>, int>,
        IPC::Reply<int>
    >;
    using ParseReadTokenMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_READ_TOKEN>, int>,
        IPC::Reply<bool, pc_token_t>
    >;
    using ParseSourceFileAndLineMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_SOURCE_FILE_AND_LINE>, int>,
        IPC::Reply<int, std::string, int>
    >;

    // Misc Syscall Definitions

    enum EngineMiscMessages {
        CREATE_SHARED_MEMORY,
        CRASH_DUMP,
    };

    // CreateSharedMemoryMsg
    using CreateSharedMemoryMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<MISC, CREATE_SHARED_MEMORY>, uint32_t>,
        IPC::Reply<IPC::SharedMemory>
    >;
    // CrashDumpMsg
    using CrashDumpMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<MISC, CRASH_DUMP>, std::vector<uint8_t>>
    >;

    // Command-Related Syscall Definitions

    enum EngineCommandMessages {
        ADD_COMMAND,
        REMOVE_COMMAND,
        ENV_PRINT,
        ENV_EXECUTE_AFTER
    };

    using AddCommandMsg = IPC::Message<IPC::Id<COMMAND, ADD_COMMAND>, std::string, std::string>;
    using RemoveCommandMsg = IPC::Message<IPC::Id<COMMAND, REMOVE_COMMAND>, std::string>;
    using EnvPrintMsg = IPC::Message<IPC::Id<COMMAND, ENV_PRINT>, std::string>;
    using EnvExecuteAfterMsg = IPC::Message<IPC::Id<COMMAND, ENV_EXECUTE_AFTER>, std::string, bool>;

    enum VMCommandMessages {
        EXECUTE,
        COMPLETE
    };

    using ExecuteMsg =  IPC::SyncMessage<
        IPC::Message<IPC::Id<COMMAND, EXECUTE>, std::string>
    >;
    using CompleteMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<COMMAND, COMPLETE>, int, std::string, std::string>,
        IPC::Reply<Cmd::CompletionResult>
    >;

    // Cvar-Related Syscall Definitions

    enum EngineCvarMessages {
        REGISTER_CVAR,
        GET_CVAR,
        SET_CVAR,
        ADD_CVAR_FLAGS
    };

    using RegisterCvarMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, REGISTER_CVAR>, std::string, std::string, int, std::string>
    >;
    using GetCvarMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, GET_CVAR>, std::string>,
        IPC::Reply<std::string>
    >;
    using SetCvarMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, SET_CVAR>, std::string, std::string>
    >;
    using AddCvarFlagsMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, ADD_CVAR_FLAGS>, std::string, int>,
        IPC::Reply<bool>
    >;

    enum VMCvarMessages {
        ON_VALUE_CHANGED
    };

    using OnValueChangedMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, ON_VALUE_CHANGED>, std::string, std::string>,
        IPC::Reply<bool, std::string>
    >;

    // Log-Related Syscall Definitions

    enum EngineLogMessage {
        DISPATCH_EVENT
    };

    using DispatchLogEventMsg = IPC::Message<IPC::Id<LOG, DISPATCH_EVENT>, std::string, int>;

    // Filesystem-Related Syscall Definitions

    enum EngineFileSystemMessages {
        FS_INITIALIZE,
        FS_HOMEPATH_OPENMODE,
        FS_HOMEPATH_FILEEXISTS,
        FS_HOMEPATH_TIMESTAMP,
        FS_HOMEPATH_MOVEFILE,
        FS_HOMEPATH_DELETEFILE,
        FS_HOMEPATH_LISTFILES,
        FS_HOMEPATH_LISTFILESRECURSIVE,
        FS_PAKPATH_OPEN,
        FS_PAKPATH_TIMESTAMP,
        FS_PAKPATH_LOADPAK
    };

    using FSInitializeMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_INITIALIZE>>,
        IPC::Reply<std::string, std::string, std::vector<FS::PakInfo>, std::vector<FS::LoadedPakInfo>, std::unordered_map<std::string, std::pair<uint32_t, FS::offset_t>>>
    >;
    using FSHomePathOpenModeMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_OPENMODE>, std::string, uint32_t>,
        IPC::Reply<Util::optional<IPC::OwnedFileHandle>>
    >;
    using FSHomePathFileExistsMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_FILEEXISTS>, std::string>,
        IPC::Reply<bool>
    >;
    using FSHomePathTimestampMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_TIMESTAMP>, std::string>,
        IPC::Reply<Util::optional<uint64_t>>
    >;
    using FSHomePathMoveFileMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_MOVEFILE>, std::string, std::string>,
        IPC::Reply<bool>
    >;
    using FSHomePathDeleteFileMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_DELETEFILE>, std::string>,
        IPC::Reply<bool>
    >;
    using FSHomePathListFilesMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_LISTFILES>, std::string>,
        IPC::Reply<Util::optional<std::vector<std::string>>>
    >;
    using FSHomePathListFilesRecursiveMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_LISTFILESRECURSIVE>, std::string>,
        IPC::Reply<Util::optional<std::vector<std::string>>>
    >;
    using FSPakPathOpenMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_OPEN>, uint32_t, std::string>,
        IPC::Reply<Util::optional<IPC::OwnedFileHandle>>
    >;
    using FSPakPathTimestampMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_TIMESTAMP>, uint32_t, std::string>,
        IPC::Reply<Util::optional<uint64_t>>
    >;
    using FSPakPathLoadPakMsg = IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_LOADPAK>, uint32_t, Util::optional<uint32_t>, std::string>
    >;

}

#endif // COMMON_COMMON_SYSCALLS_H_
