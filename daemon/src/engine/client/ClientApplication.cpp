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

#include "framework/Application.h"
#include "framework/CommandSystem.h"
#include "client.h"

#if defined(_WIN32) || defined(BUILD_CLIENT)
#include <SDL.h>
#endif

namespace Application {

class ClientApplication : public Application {
    public:
        ClientApplication() {
            #if defined(_WIN32) && defined(BUILD_TTY_CLIENT)
                // The windows dedicated server and tty client must enable the
                // curses interface because they have no other usable interface.
                traits.useCurses = true;
            #endif
            #ifdef BUILD_CLIENT
                traits.isClient = true;
            #endif
            #ifdef BUILD_TTY_CLIENT
                traits.isTTYClient = true;
            #endif
        }

        void LoadInitialConfig(bool resetConfig) override {
            //TODO(kangz) move this functions and its friends to FileSystem.cpp
	        FS_LoadBasePak();

            CL_InitKeyCommands(); // for binds

	        Cmd::BufferCommandText("preset default.cfg");
	        if (!resetConfig) {
                Cmd::BufferCommandText("exec -f " CONFIG_NAME);
                Cmd::BufferCommandText("exec -f " KEYBINDINGS_NAME);
                Cmd::BufferCommandText("exec -f " AUTOEXEC_NAME);
	        }
        }

        void Initialize(Str::StringRef uri) override {
            Com_Init((char*) "");

            if (!uri.empty()) {
                Cmd::BufferCommandTextAfter(std::string("connect ") + Cmd::Escape(uri));
            }
        }

        void Frame() override {
            Com_Frame();
        }

        void OnDrop(Str::StringRef reason) override {
            FS::PakPath::ClearPaks();
            FS_LoadBasePak();
            SV_Shutdown(Str::Format("Server crashed: %s\n", reason).c_str());
            CL_Disconnect(true);
            CL_FlushMemory();
        }

        void Shutdown(bool error, Str::StringRef message) override {
            #if defined(_WIN32) || defined(BUILD_CLIENT)
                if (error) {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PRODUCT_NAME, message.c_str(), nullptr);
                }
            #endif

            TRY_SHUTDOWN(CL_Shutdown());
            TRY_SHUTDOWN(
                SV_Shutdown(error ? Str::Format("Server fatal crashed: %s\n", message).c_str() : Str::Format("%s\n", message).c_str())
            );
            TRY_SHUTDOWN(Com_Shutdown());

            #if defined(_WIN32) || defined(BUILD_CLIENT)
                // Always run SDL_Quit, because it restores system resolution and gamma.
                SDL_Quit();
            #endif
        }

        void OnUnhandledCommand(const Cmd::Args& args) override {
            CL_ForwardCommandToServer(args.EscapedArgs(0).c_str());
        }
};

INSTANTIATE_APPLICATION(ClientApplication);

}
