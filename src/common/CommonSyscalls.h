/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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

namespace VM {

    typedef enum {
      QVM,
      COMMAND,
      CVAR,
      LOG,
      FILESYSTEM,
      LAST_COMMON_SYSCALL
    } gameServices_t;

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
        SET_CVAR
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
        FS_PAKPATH_TIMESTAMP
    };

    // FSInitializeMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_INITIALIZE>>,
        IPC::Reply<std::string, std::string, std::vector<FS::PakInfo>, std::vector<FS::PakInfo>, std::unordered_map<std::string, std::pair<uint32_t, FS::offset_t>>>
    > FSInitializeMsg;
    // FSHomePathOpenModeMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_HOMEPATH_OPENMODE>, std::string, uint32_t>,
        IPC::Reply<Util::optional<IPC::FileHandle>>
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
        IPC::Reply<Util::optional<IPC::FileHandle>>
    > FSPakPathOpenMsg;
    // FSPakPathTimestampMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC::Id<FILESYSTEM, FS_PAKPATH_TIMESTAMP>, uint32_t, std::string>,
        IPC::Reply<Util::optional<uint64_t>>
    > FSPakPathTimestampMsg;

}

#endif // COMMON_COMMON_SYSCALLS_H_
