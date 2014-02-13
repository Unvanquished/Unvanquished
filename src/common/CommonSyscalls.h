#include "IPC.h"
#include "Command.h"

namespace IPC {
    typedef enum {
      QVM,
      COMMAND,
      CVAR,
      LAST_COMMON_SYSCALL
    } gameServices_t;
}

namespace Cmd {

    // Command-Related Syscall Definitions

    enum EngineCommandRPC {
        ADD_COMMAND,
        REMOVE_COMMAND,
        ENV_PRINT,
        ENV_EXECUTE_AFTER
    };

    enum VMCommandRPC {
        EXECUTE,
        COMPLETE
    };

    typedef IPC::Message<IPC_ID(IPC::COMMAND, ADD_COMMAND), std::string, std::string>   AddCommandSyscall;
    typedef IPC::Message<IPC_ID(IPC::COMMAND, REMOVE_COMMAND), std::string>             RemoveCommandSyscall;
    typedef IPC::Message<IPC_ID(IPC::COMMAND, ENV_PRINT), std::string>                  EnvPrintSyscall;
    typedef IPC::Message<IPC_ID(IPC::COMMAND, ENV_EXECUTE_AFTER), std::string, int>     EnvExecuteAfterSyscall;

    typedef IPC::Message<IPC_ID(IPC::COMMAND, EXECUTE), std::string>                    ExecuteSyscall;
    typedef IPC::Message<IPC_ID(IPC::COMMAND, COMPLETE), int, std::string, std::string> CompleteSyscall;
    typedef IPC::Message<IPC::RETURN, Cmd::CompletionResult>                            CompleteSyscallAnswer;

}

namespace Cvar {

    // Cvar-Related Syscall Definitions

    enum EngineCommandRPC {
        REGISTER_CVAR,
        GET_CVAR,
        SET_CVAR
    };

    enum VMCommandRPC {
        ON_VALUE_CHANGED
    };

    typedef IPC::Message<IPC_ID(IPC::CVAR, REGISTER_CVAR), std::string, std::string, int, std::string> RegisterCvarSyscall;
    typedef IPC::Message<IPC_ID(IPC::CVAR, GET_CVAR), std::string>                                     GetCvarSyscall;
    typedef IPC::Message<IPC::RETURN, std::string>                                                     GetCvarSyscallAnswer;
    typedef IPC::Message<IPC_ID(IPC::CVAR, SET_CVAR), std::string, std::string>                        SetCvarSyscall;

    typedef IPC::Message<IPC_ID(IPC::CVAR, ON_VALUE_CHANGED), std::string, std::string>                 OnValueChangedSyscall;
    typedef IPC::Message<IPC::RETURN, bool, std::string>                                               OnValueChangedSyscallAnswer;

}
