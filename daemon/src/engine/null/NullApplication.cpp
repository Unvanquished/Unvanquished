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

//TODO(kangz)
#include "common/Common.h"
#include "common/System.h"
#include "framework/Application.h"
#include "framework/BaseCommands.h"
#include "framework/CommandSystem.h"
#include "qcommon/qcommon.h"

namespace Application {

class NullApplication : public Application {
    public:
        NullApplication() {
        }

        void Frame() override {
            while (true) {
                const char* command = CON_Input();
                if (command == nullptr) {
                    break;
                }

                if (command[0] == '/' || command[0] == '\\') {
                    Cmd::BufferCommandTextAfter(command + 1, true);
                } else {
                    Cmd::BufferCommandTextAfter(command, true);
                }
            }

            Cmd::DelayFrame();
            Cmd::ExecuteCommandBuffer();

            Sys::SleepFor(std::chrono::milliseconds(60));
        }
};

INSTANTIATE_APPLICATION(NullApplication);

}
