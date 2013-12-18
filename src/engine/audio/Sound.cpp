/*
===========================================================================

daemon gpl source code
copyright (c) 2013 unvanquished developers

this file is part of the daemon gpl source code (daemon source code).

daemon source code is free software: you can redistribute it and/or modify
it under the terms of the gnu general public license as published by
the free software foundation, either version 3 of the license, or
(at your option) any later version.

daemon source code is distributed in the hope that it will be useful,
but without any warranty; without even the implied warranty of
merchantability or fitness for a particular purpose.  see the
gnu general public license for more details.

you should have received a copy of the gnu general public license
along with daemon source code.  if not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "AudioPrivate.h"

namespace Audio {

    //TODO nice usecase for Cvar::Range
    static Cvar::Cvar<float> effectsVolume("sound.volume.effects", "the volume of the effects", Cvar::ARCHIVE, 0.8f);
    static Cvar::Cvar<float> musicVolume("sound.volume.music", "the volume of the music", Cvar::ARCHIVE, 0.8f);

    struct sourceRecord_t {
        AL::Source source;
        std::shared_ptr<Sound> usingSound;
        bool active;
        int priority;
    };

    static sourceRecord_t* sources = nullptr;
    static constexpr int nSources = 128; //TODO see what's the limit for OpenAL soft

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

    void AddSound(std::shared_ptr<Emitter> emitter, std::shared_ptr<Sound> sound, int priority) {
        sourceRecord_t* source = GetSource(priority);

        if (source) {
            sound->SetEmitter(emitter);
            sound->AcquireSource(source->source);
            source->usingSound = sound;
            source->priority = priority;
            source->active = true;

            sound->FinishSetup();
            sound->Play();
        }
    }

    sourceRecord_t* GetSource(int priority) {
        //TODO make a better heuristic? (take into account the distance / the volume /... ?)
        int best = -1;
        int bestPriority = priority;

        for (int i = 0; i < nSources; i++) {
            sourceRecord_t& source = sources[i];

            if (not source.active) {
                return &source;
            }

            if (best < 0 && source.priority <= priority) {
                best = i;
                bestPriority = source.priority;
                continue;
            }

            if (source.priority < bestPriority) {
                best = i;
                bestPriority = source.priority;
                continue;
            }
        }

        if (best >= 0) {
            sourceRecord_t& source = sources[best];

            source.usingSound = nullptr;
            return &source;
        } else {
            return nullptr;
        }
    }

    // Implementation of Sound

    Sound::Sound(): positionalGain(1.0f), soundGain(1.0f), currentGain(1.0f) {
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

    void Sound::FinishSetup() {
        currentGain = positionalGain * soundGain * effectsVolume.Get();
        source->SetGain(currentGain);
    }

    void Sound::Update() {
        float targetGain = positionalGain * soundGain * effectsVolume.Get();

        //TODO make it framerate dependant and fade out in about 1/8 seconds ?
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

    OneShotSound::OneShotSound(Sample* sample): sample(sample) {
    }

    OneShotSound::~OneShotSound() {
    }

    void OneShotSound::SetupSource(AL::Source& source) {
        source.SetBuffer(sample->GetBuffer());
    }

    void OneShotSound::InternalUpdate() {
        if (GetSource().IsStopped()) {
            Stop();
        }
    }

    // Implementation of LoopingSound

    LoopingSound::LoopingSound(Sample* sample): sample(sample), fadingOut(false) {
    }

    LoopingSound::~LoopingSound() {
    }

    void LoopingSound::FadeOutAndDie() {
        fadingOut = true;
        SetSoundGain(0.0f);
    }

    void LoopingSound::SetupSource(AL::Source& source) {
        source.SetLooping(true);
        source.SetBuffer(sample->GetBuffer());
    }

    void LoopingSound::InternalUpdate() {
        if (fadingOut and GetCurrentGain() == 0.0f) {
            Stop();
        }
    }

}
