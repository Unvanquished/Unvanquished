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

#include "Application.h"

namespace Application {

    Traits::Traits()
    : isClient(false), isTTYClient(false), isServer(false), useCurses(false), supportsUri(false) {
    }

    // Forward declaration of the function declared by INSTANTIATE_APPLICATION
    Application& GetApp();

    Application::Application()
    {
    }

    void Application::LoadInitialConfig(bool) {
    }

    void Application::Initialize(Str::StringRef) {
    }

    void Application::OnDrop(Str::StringRef) {
    }

    void Application::Shutdown(bool, Str::StringRef) {
    }

    void Application::OnUnhandledCommand(const Cmd::Args& args) {
        Log::Notice("Unknown command '%s'", args[0]);
    }

    const Traits& Application::GetTraits() const {
        return traits;
    }

    void LoadInitialConfig(bool resetConfig) {
        GetApp().LoadInitialConfig(resetConfig);
    }

    void Initialize(Str::StringRef uri) {
        GetApp().Initialize(uri);
    }

    void Frame() {
        GetApp().Frame();
    }

    void OnDrop(Str::StringRef reason) {
        GetApp().OnDrop(reason);
    }

    void Shutdown(bool error, Str::StringRef message) {
        GetApp().Shutdown(error, message);
    }

    void OnUnhandledCommand(const Cmd::Args& args) {
        GetApp().OnUnhandledCommand(args);
    }

    const Traits& GetTraits() {
        return GetApp().GetTraits();
    }
}

