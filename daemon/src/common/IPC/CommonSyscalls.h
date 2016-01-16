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

    // PrintMsg
    typedef IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PRINT>, std::string> PrintMsg;
    // ErrorMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_ERROR>, std::string>
    > ErrorMsg;
    // LogMsg TODO
    // SendConsoleCommandMsg
    typedef IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_SEND_CONSOLE_COMMAND>, std::string> SendConsoleCommandMsg;

    // FSFOpenFileMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FOPEN_FILE>, std::string, bool, int>,
        IPC::Reply<int, int>
    > FSFOpenFileMsg;
    // FSReadMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_READ>, int, int>,
        IPC::Reply<std::string, int>
    > FSReadMsg;
    // FSWriteMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_WRITE>, int, std::string>,
        IPC::Reply<int>
    > FSWriteMsg;
    // FSSeekMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_SEEK>, int, int, int>,
        IPC::Reply<int>
    > FSSeekMsg;
    // FSTellMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_TELL>, fileHandle_t>,
        IPC::Reply<int>
    > FSTellMsg;
	// FSFileLengthMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FILELENGTH>, fileHandle_t>,
		IPC::Reply<int>
	> FSFileLengthMsg;
    // FSRenameMsg
    typedef IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_RENAME>, std::string, std::string> FSRenameMsg;
    // FSFCloseFile
    typedef IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FCLOSE_FILE>, int> FSFCloseFileMsg;
    // FSGetFileListMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_GET_FILE_LIST>, std::string, std::string, int>,
        IPC::Reply<int, std::string>
    > FSGetFileListMsg;
    // FSGetFileListRecursiveMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_GET_FILE_LIST_RECURSIVE>, std::string, std::string, int>,
        IPC::Reply<int, std::string>
    > FSGetFileListRecursiveMsg;
    // FSFindPakMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_FIND_PAK>, std::string>,
        IPC::Reply<bool>
    > FSFindPakMsg;
    // FSLoadPakMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_LOAD_PAK>, std::string, std::string>,
        IPC::Reply<bool>
    > FSLoadPakMsg;
    // FSLoadMapMetadataMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_FS_LOAD_MAP_METADATA>>
    > FSLoadMapMetadataMsg;

    //ParseAddGlobalDefineMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_ADD_GLOBAL_DEFINE>, std::string>,
        IPC::Reply<int>
    > ParseAddGlobalDefineMsg;
    //ParseLoadSourceMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_LOAD_SOURCE>, std::string>,
        IPC::Reply<int>
    > ParseLoadSourceMsg;
    //ParseFreeSourceMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_FREE_SOURCE>, int>,
        IPC::Reply<int>
    > ParseFreeSourceMsg;
    //ParseReadTokenMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_READ_TOKEN>, int>,
        IPC::Reply<bool, pc_token_t>
    > ParseReadTokenMsg;
    //ParseSourceFileAndLineMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<VM::QVM_COMMON, QVM_COMMON_PARSE_SOURCE_FILE_AND_LINE>, int>,
        IPC::Reply<int, std::string, int>
    > ParseSourceFileAndLineMsg;

    // Misc Syscall Definitions

    enum EngineMiscMessages {
        CREATE_SHARED_MEMORY
    };

    // CreateSharedMemoryMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<MISC, CREATE_SHARED_MEMORY>, uint32_t>,
        IPC::Reply<IPC::SharedMemory>
    > CreateSharedMemoryMsg;

    // Command-Related Syscall Definitions

    enum EngineCommandMessages {
        ADD_COMMAND,
        REMOVE_COMMAND,
        ENV_PRINT,
        ENV_EXECUTE_AFTER
    };

    // AddCommandMsg
    typedef IPC::Message<IPC::Id<COMMAND, ADD_COMMAND>, std::string, std::string> AddCommandMsg;
    // RemoveCommandMsg
    typedef IPC::Message<IPC::Id<COMMAND, REMOVE_COMMAND>, std::string> RemoveCommandMsg;
    // EnvPrintMsg
    typedef IPC::Message<IPC::Id<COMMAND, ENV_PRINT>, std::string> EnvPrintMsg;
    // EnvExecuteAfterMsg
    typedef IPC::Message<IPC::Id<COMMAND, ENV_EXECUTE_AFTER>, std::string, bool> EnvExecuteAfterMsg;

    enum VMCommandMessages {
        EXECUTE,
        COMPLETE
    };

    // ExecuteMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<COMMAND, EXECUTE>, std::string>
    >ExecuteMsg;
    // CompleteMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<COMMAND, COMPLETE>, int, std::string, std::string>,
        IPC::Reply<Cmd::CompletionResult>
    > CompleteMsg;

    // Cvar-Related Syscall Definitions

    enum EngineCvarMessages {
        REGISTER_CVAR,
        GET_CVAR,
        SET_CVAR,
        ADD_CVAR_FLAGS
    };

    // RegisterCvarMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, REGISTER_CVAR>, std::string, std::string, int, std::string>
    > RegisterCvarMsg;
    // GetCvarMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, GET_CVAR>, std::string>,
        IPC::Reply<std::string>
    > GetCvarMsg;
    // SetCvarMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, SET_CVAR>, std::string, std::string>
    > SetCvarMsg;
    // SetCvarMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, ADD_CVAR_FLAGS>, std::string, int>,
        IPC::Reply<bool>
    > AddCvarFlagsMsg;

    enum VMCvarMessages {
        ON_VALUE_CHANGED
    };

    // OnValueChangedMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<CVAR, ON_VALUE_CHANGED>, std::string, std::string>,
        IPC::Reply<bool, std::string>
    > OnValueChangedMsg;

    // Log-Related Syscall Definitions

    enum EngineLogMessage {
        DISPATCH_EVENT
    };

    // DispatchLogEventMsg
    typedef IPC::Message<IPC::Id<LOG, DISPATCH_EVENT>, std::string, int> DispatchLogEventMsg;

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

    // FSInitializeMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_INITIALIZE>>,
        IPC::Reply<std::string, std::string, std::vector<FS::PakInfo>, std::vector<FS::LoadedPakInfo>, std::unordered_map<std::string, std::pair<uint32_t, FS::offset_t>>>
    > FSInitializeMsg;
    // FSHomePathOpenModeMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_OPENMODE>, std::string, uint32_t>,
        IPC::Reply<Util::optional<IPC::OwnedFileHandle>>
    > FSHomePathOpenModeMsg;
    // FSHomePathFileExistsMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_FILEEXISTS>, std::string>,
        IPC::Reply<bool>
    > FSHomePathFileExistsMsg;
    // FSHomePathTimestampMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_TIMESTAMP>, std::string>,
        IPC::Reply<Util::optional<uint64_t>>
    > FSHomePathTimestampMsg;
    // FSHomePathMoveFileMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_MOVEFILE>, std::string, std::string>,
        IPC::Reply<bool>
    > FSHomePathMoveFileMsg;
    // FSHomePathDeleteFileMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_DELETEFILE>, std::string>,
        IPC::Reply<bool>
    > FSHomePathDeleteFileMsg;
    // FSHomePathListFilesMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_LISTFILES>, std::string>,
        IPC::Reply<Util::optional<std::vector<std::string>>>
    > FSHomePathListFilesMsg;
    // FSHomePathListFilesRecursiveMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_LISTFILESRECURSIVE>, std::string>,
        IPC::Reply<Util::optional<std::vector<std::string>>>
    > FSHomePathListFilesRecursiveMsg;
    // FSPakPathOpenMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_OPEN>, uint32_t, std::string>,
        IPC::Reply<Util::optional<IPC::OwnedFileHandle>>
    > FSPakPathOpenMsg;
    // FSPakPathTimestampMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_TIMESTAMP>, uint32_t, std::string>,
        IPC::Reply<Util::optional<uint64_t>>
    > FSPakPathTimestampMsg;
    // FSPakPathLoadPakMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_LOADPAK>, uint32_t, Util::optional<uint32_t>, std::string>
    > FSPakPathLoadPakMsg;

}

#endif // COMMON_COMMON_SYSCALLS_H_
