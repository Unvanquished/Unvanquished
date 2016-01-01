/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2015, Daemon Developers
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
#include "qcommon/qcommon.h"

namespace Application {

class ServerApplication : public Application {
    public:
        ServerApplication() {
            #ifdef _WIN32
                // The windows dedicated server and tty client must enable the
                // curses interface because they have no other usable interface.
                traits.useCurses = true;
            #endif
            traits.isServer = true;
            traits.uniqueHomepathSuffix = "-server";
        }

        void LoadInitialConfig(bool resetConfig) override {
            //TODO(kangz) move this functions and its friends to FileSystem.cpp
	        FS_LoadBasePak();
	        if (!resetConfig) {
		        Cmd::BufferCommandText("exec -f " SERVERCONFIG_NAME);
	        }
        }

        void Initialize(Str::StringRef) override {
            Com_Init((char*) "");
        }

        void Frame() override {
            Com_Frame();
        }

        void OnDrop(Str::StringRef reason) override {
            FS::PakPath::ClearPaks();
            FS_LoadBasePak();
            SV_Shutdown(va("********************\nServer crashed: %s\n********************\n", reason.c_str()));
        }

        void Shutdown(bool error, Str::StringRef message) override {
            TRY_SHUTDOWN(
                SV_Shutdown(error ? va("Server fatal crashed: %s\n", message.c_str()) : va("%s\n", message.c_str()))
            );
            TRY_SHUTDOWN(Com_Shutdown());
        }
};

INSTANTIATE_APPLICATION(ServerApplication);

}
