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

#include "ALObjects.h"
#include "Emitter.h"
#include "Sample.h"
#include "../../common/Log.h"

namespace Audio {

    struct sourceRecord_t {
        AL::Source source;
        Sound* usingSound;
        bool active;
        int priority;
        int lastUsedTime;
    };

    static sourceRecord_t* sources = nullptr;
    static constexpr int nSources = 128; //TODO see what's the limit for OpenAL soft

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

            if (source.lastUsedTime < sources[best].lastUsedTime) {
                best = i;
                continue;
            }
        }

        if (best >= 0) {
            sourceRecord_t& source = sources[best];

            delete source.usingSound;
            return &source;
        } else {
            return nullptr;
        }
    }

    // Implementation of Sound

    Sound::Sound() {
    }

    Sound::~Sound() {
        source->active = false;
        source->usingSound = nullptr;
        source->source.Stop();

        emitter->RemoveSound(this);
    }

    void Sound::Play() {
        source->source.Play();
    }

    void Sound::AcquireSource(sourceRecord_t* source) {
        this->source = source;

        AL::Source& alSource = source->source;
        alSource.SetLooping(false);

        SetupSource(source->source);
        emitter->SetupSource(source->source);
    }

    AL::Source& Sound::GetSource() {
        return source->source;
    }

    void Sound::SetEmitter(Emitter* emitter) {
        this->emitter = emitter;
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
            delete this;
        }
    }

    // Implementation of LoopingSound
/*
    LoopingSound::LoopingSound(Sample* sample): sample(sample) {
    }

    LoopingSound::~LoopingSound() {
    }

    void LoopingSound::SetupSource(AL::Source& source) {
        source.SetLooping(true);
        source.SetBuffer(sample->GetBuffer());
    }
*/
    // Exposed interface implementation

    void AddSound(Emitter* emitter, Sound* sound, int priority) {
        sourceRecord_t* source = GetSource(priority);

        if (source) {
            emitter->AddSound(sound);

            sound->AcquireSource(source);
            source->usingSound = sound;
            source->priority = priority;
            source->active = true;

            sound->Play();
        } else {
            delete sound;
        }
    }

    void AddSoundHelper(int entityNum, const vec3_t origin, Sound* sound, int priority) {
        Emitter* emitter;

        // Apparently no origin means it is attached to an entity
        if (not origin) {
            emitter = GetEmitterForEntity(entityNum);

        } else {
            emitter = GetEmitterForPosition(origin);
        }

        AddSound(emitter, sound, priority);
    }

    void StartSound(int entityNum, const vec3_t origin, Sample* sample) {
        Log::Debug("Adding sound %s for entity %i", sample->GetName(), entityNum);
        AddSoundHelper(entityNum, origin, new OneShotSound(sample), 1);
    }

    void StartLocalSound(Sample* sample) {
        Log::Debug("Adding sound %s", sample->GetName());
        AddSound(GetLocalEmitter(), new OneShotSound(sample), 1);
    }

    void AddAmbientLoopingSound(int entityNum, const vec3_t origin, Sample* sample) {
        //AddSoundHelper(entityNum, origin, new LoopingSound(sample), 1);
    }
    void AddEntityLoopingSound(int entityNum, const vec3_t origin, Sample* sample) {
        //AddSoundHelper(entityNum, origin, new LoopingSound(sample), 1);
    }

    void ClearEmitterLoopingSounds(Emitter* emitter) {
        std::vector<Sound*> sounds = emitter->GetSounds();

        for (Sound* sound : sounds) {
            //TODO
        }
    }

    void ClearAllLoopingSounds() {
        //TODO
    }

    void ClearLoopingSoundsForEntity(int entityNum) {
        ClearEmitterLoopingSounds(GetEmitterForEntity(entityNum));
    }
}
