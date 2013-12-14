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

#include "Sound.h"

//TODO remove me
#include "API.h"

#include "ALObjects.h"
#include "Emitter.h"
#include "Sample.h"
#include "../../common/Log.h"

namespace Audio {

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

    void AddSound(Emitter* emitter, std::shared_ptr<Sound> sound, int priority) {
        sourceRecord_t* source = GetSource(priority);

        if (source) {
            sound->SetEmitter(emitter);
            sound->AcquireSource(source->source);
            source->usingSound = sound;
            source->priority = priority;
            source->active = true;

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

    Sound::Sound() {
    }

    Sound::~Sound() {
        emitter->Release();
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

    void Sound::SetEmitter(Emitter* emitter) {
        this->emitter = emitter;
        emitter->Retain();
    }

    Emitter* Sound::GetEmitter() {
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

    // Implementation of OneShotSound

    OneShotSound::OneShotSound(Sample* sample): sample(sample) {
    }

    OneShotSound::~OneShotSound() {
    }

    void OneShotSound::SetupSource(AL::Source& source) {
        source.SetBuffer(sample->GetBuffer());
    }

    void OneShotSound::Update() {
        if (GetSource().IsStopped()) {
            Stop();
        }
    }

    // Implementation of LoopingSound

    LoopingSound::LoopingSound(Sample* sample): sample(sample) {
    }

    LoopingSound::~LoopingSound() {
    }

    void LoopingSound::SetupSource(AL::Source& source) {
        source.SetLooping(true);
        source.SetBuffer(sample->GetBuffer());
    }

    void LoopingSound::Update() {
    }

}
