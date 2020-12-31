/*
===========================================================================

Unvanquished BSD Source Code
Copyright (c) 2013-2016, Unvanquished Developers
All rights reserved.

This file is part of the Unvanquished BSD Source Code (Unvanquished Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Unvanquished developers nor the
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

#include "cg_local.h"
#include "engine/client/cg_msgdef.h"

#include "shared/VMMain.h"
#include "shared/CommandBufferClient.h"
#include "shared/CommonProxies.h"
#include "shared/client/cg_api.h"

// Symbols required by the shared VMMain code

int VM::VM_API_VERSION = CGAME_API_VERSION;

void CG_Init( int serverMessageNum, int clientNum, const glconfig_t& gl, const GameStateCSs& gameState );
void CG_Shutdown();

void VM::VMHandleSyscall(uint32_t id, Util::Reader reader) {
    int major = id >> 16;
    int minor = id & 0xffff;
    if (major == VM::QVM) {
        switch (minor) {
            case CG_STATIC_INIT:
                IPC::HandleMsg<CGameStaticInitMsg>(VM::rootChannel, std::move(reader), [] (int milliseconds) {
                    VM::InitializeProxies(milliseconds);
                    FS::Initialize();
                    srand(time(nullptr));
		    cmdBuffer.Init();
                });
                break;

            case CG_INIT:
                IPC::HandleMsg<CGameInitMsg>(VM::rootChannel, std::move(reader), [] (int serverMessageNum, int clientNum, const glconfig_t& gl, const GameStateCSs& gamestate) {
                    CG_Init(serverMessageNum, clientNum, gl, gamestate);
                    cmdBuffer.TryFlush();
                });
                break;

            case CG_SHUTDOWN:
                IPC::HandleMsg<CGameShutdownMsg>(VM::rootChannel, std::move(reader), [] {
                    CG_Shutdown();
                });
                break;

			case CG_ROCKET_VM_INIT:
				IPC::HandleMsg<CGameRocketInitMsg>(VM::rootChannel, std::move(reader), [] (glconfig_t gl) {
					CG_Rocket_Init(gl);
				});
				break;

			case CG_ROCKET_FRAME:
				IPC::HandleMsg<CGameRocketFrameMsg>(VM::rootChannel, std::move(reader), [] (cgClientState_t cs) {
					CG_Rocket_Frame(cs);
					cmdBuffer.TryFlush();
				});
				break;

            case CG_DRAW_ACTIVE_FRAME:
                IPC::HandleMsg<CGameDrawActiveFrameMsg>(VM::rootChannel, std::move(reader), [] (int serverTime, bool demoPlayback) {
                    CG_DrawActiveFrame(serverTime, demoPlayback);
                    cmdBuffer.TryFlush();
                });
                break;

            case CG_KEY_EVENT:
                IPC::HandleMsg<CGameKeyEventMsg>(VM::rootChannel, std::move(reader), [] (Keyboard::Key key, bool down) {
                    CG_KeyEvent(key, down);
                    cmdBuffer.TryFlush();
                });
                break;

            case CG_MOUSE_EVENT:
                IPC::HandleMsg<CGameMouseEventMsg>(VM::rootChannel, std::move(reader), [] (int dx, int dy) {
                    CG_MouseEvent(dx, dy);
					cmdBuffer.TryFlush();
                });
                break;

			case CG_MOUSE_POS_EVENT:
				IPC::HandleMsg<CGameMousePosEventMsg>(VM::rootChannel, std::move(reader), [] (int x, int y) {
					CG_MousePosEvent(x, y);
					cmdBuffer.TryFlush();
				});
				break;

			case CG_CHARACTER_INPUT_EVENT:
				IPC::HandleMsg<CGameCharacterInputMsg>(VM::rootChannel, std::move(reader), [] (int c) {
					Rocket_ProcessTextInput(c);
					cmdBuffer.TryFlush();
				});
				break;

			case CG_CONSOLE_LINE:
				IPC::HandleMsg<CGameConsoleLineMsg>(VM::rootChannel, std::move(reader), [](std::string str) {
					Rocket_AddConsoleText( str );
					cmdBuffer.TryFlush();
				});
				break;

			case CG_FOCUS_EVENT:
				IPC::HandleMsg<CGameFocusEventMsg>(VM::rootChannel, std::move(reader), [] (bool focus) {
					CG_FocusEvent(focus);
					cmdBuffer.TryFlush();
				});
				break;

            default:
                Sys::Drop("VMMain(): unknown cgame command %i", minor);

        }
    } else if (major < VM::LAST_COMMON_SYSCALL) {
        VM::HandleCommonSyscall(major, minor, std::move(reader), VM::rootChannel);
    } else {
        Sys::Drop("unhandled VM major syscall number %i", major);
    }
}
