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

#ifndef AUDIO_AUDIO_H_
#define AUDIO_AUDIO_H_

#include "common/Common.h"

namespace Audio {

    // An interface to the sound system that mimics the previous sound system's behavior

    bool Init();
    void Shutdown();
    void Update();

    void BeginRegistration();
    sfxHandle_t RegisterSFX(Str::StringRef filename);
    void EndRegistration();

    void StartSound(int entityNum, Vec3 origin, sfxHandle_t sfx);
    void StartLocalSound(int entityNum);

    void AddEntityLoopingSound(int entityNum, sfxHandle_t sfx);
    void ClearAllLoopingSounds();
    void ClearLoopingSoundsForEntity(int entityNum);

    void StartMusic(Str::StringRef leadingSound, Str::StringRef loopSound);
    void StopMusic();

    void StopAllSounds();

    void StreamData(int streamNum, const void* data, int numSamples, int rate, int width, int channels, float volume, int entityNum);

    void UpdateListener(int entityNum, const Vec3 orientation[3]);
    void UpdateListenerGain();
    void UpdateEntityPosition(int entityNum, Vec3 position);
    void UpdateEntityVelocity(int entityNum, Vec3 velocity);

    void SetReverb(int slotNum, std::string name, float ratio);

    void StartCapture(int rate);
    int AvailableCaptureSamples();
    void GetCapturedData(int numSamples, void* buffer);
    void StopCapture();
}

#endif //AUDIO_AUDIO_H_
