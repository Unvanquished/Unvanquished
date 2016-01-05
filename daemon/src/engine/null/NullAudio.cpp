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

#include "common/Common.h"

namespace Audio {

    bool Init() {
        return true;
    }

    void Shutdown() {
    }

    void Update() {
    }


    void BeginRegistration() {
    }

    sfxHandle_t RegisterSFX(Str::StringRef) {
        return 0;
    }

    void EndRegistration() {
    }


    void StartSound(int, Vec3, sfxHandle_t) {
    }

    void StartLocalSound(int) {
    }


    void AddEntityLoopingSound(int, sfxHandle_t) {
    }

    void ClearAllLoopingSounds() {
    }

    void ClearLoopingSoundsForEntity(int) {
    }


    void StartMusic(Str::StringRef, Str::StringRef) {
    }

    void StopMusic() {
    }


    void StopAllSounds() {
    }


    void StreamData(int, const void*, int, int, int, int, float, int) {
    }


    void UpdateListener(int, const Vec3[3]) {
    }

    void UpdateEntityPosition(int, Vec3) {
    }

    void UpdateEntityVelocity(int, Vec3) {
    }


    void SetReverb(int, std::string, float) {
    }


    void StartCapture(int) {
    }

    int AvailableCaptureSamples() {
        return 0;
    }

    void GetCapturedData(int, void*) {
    }

    void StopCapture() {
    }
}
