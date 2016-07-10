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

#include "AudioPrivate.h"

namespace Audio {

    //TODO lazily check for the values
    static Cvar::Range<Cvar::Cvar<float>> effectsVolume("audio.volume.effects", "the volume of the effects", Cvar::NONE, 0.8f, 0.0f, 1.0f);
    static Cvar::Range<Cvar::Cvar<float>> musicVolume("audio.volume.music", "the volume of the music", Cvar::NONE, 0.8f, 0.0f, 1.0f);

    // We have a big, fixed number of source to avoid rendering too many sounds and slowing down the rest of the engine.
    struct sourceRecord_t {
        AL::Source source;
        std::shared_ptr<Sound> usingSound;
        bool active;
        int priority;
    };

    static sourceRecord_t* sources = nullptr;
    static CONSTEXPR int nSources = 128; //TODO see what's the limit for OpenAL soft

    sourceRecord_t* GetSource(int priority);

    static bool initialized = false;

    void InitSounds() {
        if (initialized) {
            return;
        }

        sources = new sourceRecord_t[nSources];

        for (int i = 0; i < nSources; i++) {
            sources[i].active = false;
        }

        initialized = true;
    }

    void ShutdownSounds() {
        if (not initialized) {
            return;
        }

        delete[] sources;
        sources = nullptr;

        initialized = false;
    }

    void UpdateSounds() {
        for (int i = 0; i < nSources; i++) {
            if (sources[i].active) {
                auto sound = sources[i].usingSound;

                // Update and Emitter::UpdateSound can call Sound::Stop
                if (not sound->IsStopped()) {
                    sound->Update();
                }

                if (not sound->IsStopped()) {
                    sound->GetEmitter()->UpdateSound(*sound);
                }

                if (sound->IsStopped()) {
                    sources[i].active = false;
                    sources[i].usingSound = nullptr;
                }
            }
        }
    }

    void StopSounds() {
        for (int i = 0; i < nSources; i++) {
            if (sources[i].active) {
                sources[i].usingSound->Stop();
            }
        }
    }

    void AddSound(std::shared_ptr<Emitter> emitter, std::shared_ptr<Sound> sound, int priority) {
        sourceRecord_t* source = GetSource(priority);

        if (source) {
            // Make the source forget if it was a "static" or a "streaming" source.
            source->source.ResetBuffer();
            sound->SetEmitter(emitter);
            sound->AcquireSource(source->source);
            source->usingSound = sound;
            source->priority = priority;
            source->active = true;

            sound->FinishSetup();
            sound->Play();
        }
    }

    // Finds a inactive or low-priority source to play a new sound.
    sourceRecord_t* GetSource(int priority) {
        //TODO make a better heuristic? (take into account the distance / the volume /... ?)
        int best = -1;
        int bestPriority = priority;

        // Gets the minimum sound by comparing activity first then priority
        for (int i = 0; i < nSources; i++) {
            sourceRecord_t& source = sources[i];

            if (not source.active) {
                return &source;
            }

            if (source.priority < bestPriority || (best < 0 && source.priority <= priority)) {
                best = i;
                bestPriority = source.priority;
                continue;
            }
        }

        if (best >= 0) {
            sourceRecord_t& source = sources[best];

            source.source.Stop();
            source.source.RemoveAllQueuedBuffers();

            source.usingSound = nullptr;
            return &source;
        } else {
            return nullptr;
        }
    }

    // Implementation of Sound

    Sound::Sound(): positionalGain(1.0f), soundGain(1.0f), currentGain(1.0f), playing(false), source(nullptr) {
    }

    Sound::~Sound() {
    }

    void Sound::Play() {
        source->Play();
        playing = true;
    }

    void Sound::Stop() {
        source->Stop();
        playing = false;
    }

    bool Sound::IsStopped() {
        return not playing;
    }

    void Sound::SetPositionalGain(float gain) {
        positionalGain = gain;
    }

    void Sound::SetSoundGain(float gain) {
        soundGain = gain;
    }

    float Sound::GetCurrentGain() {
        return currentGain;
    }

    void Sound::SetEmitter(std::shared_ptr<Emitter> emitter) {
        this->emitter = emitter;
    }

    std::shared_ptr<Emitter> Sound::GetEmitter() {
        return emitter;
    }

    void Sound::AcquireSource(AL::Source& source) {
        this->source = &source;

        source.SetLooping(false);

        SetupSource(source);
        emitter->SetupSound(*this);
    }

    AL::Source& Sound::GetSource() {
        return *source;
    }

    // Set the gain before the source is started to avoid having a few milliseconds of very lound sound
    void Sound::FinishSetup() {
        currentGain = positionalGain * soundGain * SliderToAmplitude(effectsVolume.Get());
        source->SetGain(currentGain);
    }

    void Sound::Update() {
        // Fade the Gain update to avoid "ticking" sounds when there is a gain discontinuity
        float targetGain = positionalGain * soundGain * SliderToAmplitude(effectsVolume.Get());

        //TODO make it framerate independant and fade out in about 1/8 seconds ?
        if (currentGain > targetGain) {
            currentGain = std::max(currentGain - 0.02f, targetGain);
            //currentGain = std::max(currentGain * 1.05f, targetGain);
        } else if (currentGain < targetGain) {
            currentGain = std::min(currentGain + 0.02f, targetGain);
            //currentGain = std::min(currentGain / 1.05f - 0.01f, targetGain);
        }

        source->SetGain(currentGain);

        InternalUpdate();
    }
    // Implementation of OneShotSound

    OneShotSound::OneShotSound(std::shared_ptr<Sample> sample): sample(sample) {
    }

    OneShotSound::~OneShotSound() {
    }

    void OneShotSound::SetupSource(AL::Source& source) {
        source.SetBuffer(sample->GetBuffer());
        SetSoundGain(effectsVolume.Get());
    }

    void OneShotSound::InternalUpdate() {
        if (GetSource().IsStopped()) {
            Stop();
            return;
        }
        SetSoundGain(effectsVolume.Get());
    }

    // Implementation of LoopingSound

    LoopingSound::LoopingSound(std::shared_ptr<Sample> loopingSample, std::shared_ptr<Sample> leadingSample)
        : loopingSample(loopingSample),
          leadingSample(leadingSample),
          fadingOut(false) {}

    LoopingSound::~LoopingSound() {
    }

    void LoopingSound::FadeOutAndDie() {
        fadingOut = true;
        SetSoundGain(0.0f);
    }

    void LoopingSound::SetupSource(AL::Source& source) {
        if (leadingSample) {
            source.SetBuffer(leadingSample->GetBuffer());
        } else {
            SetupLoopingSound(source);
        }
        SetSoundGain(effectsVolume.Get());
    }

    void LoopingSound::InternalUpdate() {
        if (fadingOut and GetCurrentGain() == 0.0f) {
            Stop();
        }

        if (not fadingOut) {
            if (leadingSample) {
                if (GetSource().IsStopped()) {
                    SetupLoopingSound(GetSource());
                    GetSource().Play();
                    leadingSample = nullptr;
                }
            }
            SetSoundGain(effectsVolume.Get());
        }
    }

    void LoopingSound::SetupLoopingSound(AL::Source& source){
        source.SetLooping(true);
        if (loopingSample) {
            source.SetBuffer(loopingSample->GetBuffer());
        }
    }

    // Implementation of StreamingSound

    StreamingSound::StreamingSound() {
    }

    StreamingSound::~StreamingSound() {
    }

    void StreamingSound::SetupSource(AL::Source&) {
    }

    void StreamingSound::InternalUpdate() {
        AL::Source& source = GetSource();

        while (source.GetNumProcessedBuffers() > 0) {
            source.PopBuffer();
        }

        if (source.GetNumQueuedBuffers() == 0) {
            Stop();
        }
    }

    //TODO somehow try to catch back when data is coming faster than we consume (e.g. capture data)
    void StreamingSound::AppendBuffer(AL::Buffer buffer) {
        if (IsStopped()) {
            return;
        }

        AL::Source& source = GetSource();
        source.QueueBuffer(std::move(buffer));

        if (source.IsStopped()) {
            source.Play();
        }
    }

    void StreamingSound::SetGain(float gain) {
        SetSoundGain(gain);
    }
}
