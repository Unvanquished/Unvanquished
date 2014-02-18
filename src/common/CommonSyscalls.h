#include "IPC.h"
#include "Command.h"

namespace VM {

    typedef enum {
      QVM,
      COMMAND,
      CVAR,
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
    typedef IPC::Message<IPC_ID(COMMAND, ADD_COMMAND), std::string, std::string> AddCommandMsg;
    // RemoveCommandMsg
    typedef IPC::Message<IPC_ID(COMMAND, REMOVE_COMMAND), std::string> RemoveCommandMsg;
    // EnvPrintMsg
    typedef IPC::Message<IPC_ID(COMMAND, ENV_PRINT), std::string> EnvPrintMsg;
    // EnvExecuteAfterMsg
    typedef IPC::Message<IPC_ID(COMMAND, ENV_EXECUTE_AFTER), std::string, int> EnvExecuteAfterMsg;

    enum VMCommandMessages {
        EXECUTE,
        COMPLETE
    };

    // ExecuteMsg
    typedef IPC::Message<IPC_ID(COMMAND, EXECUTE), std::string> ExecuteMsg;
    // CompleteMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC_ID(COMMAND, COMPLETE), int, std::string, std::string>,
        IPC::Reply<Cmd::CompletionResult>
    > CompleteMsg;

    // Cvar-Related Syscall Definitions

    enum EngineCvarMessages {
        REGISTER_CVAR,
        GET_CVAR,
        SET_CVAR
    };

    // RegisterCvarMsg
    typedef IPC::Message<IPC_ID(CVAR, REGISTER_CVAR), std::string, std::string, int, std::string> RegisterCvarMsg;
    // GetCvarMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC_ID(CVAR, GET_CVAR), std::string>,
        IPC::Reply<std::string>
    > GetCvarMsg;
    // SetCvarMsg
    typedef IPC::Message<IPC_ID(CVAR, SET_CVAR), std::string, std::string> SetCvarMsg;

    enum VMCvarMessages {
        ON_VALUE_CHANGED
    };

    // OnValueChangedMsg
    typedef IPC::SyncMessage<
        IPC::Message<IPC_ID(CVAR, ON_VALUE_CHANGED), std::string, std::string>,
        IPC::Reply<bool, std::string>
    > OnValueChangedMsg;

}
