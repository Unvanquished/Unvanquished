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

#include "API.h"

#include "Emitter.h"
#include "Sample.h"
#include "Sound.h"

#include "../../common/Log.h"

//TODO check all inputs for validity
namespace Audio {

    void Init() {
        InitSamples();
        InitSounds();
        InitEmitters();
    }

    void Shutdown() {
        ShutdownEmitters();
        ShutdownSounds();
        ShutdownSamples();
    }

    void Update() {
        UpdateEmitters();
        UpdateSounds();
    }

    sfxHandle_t RegisterSFX(Str::StringRef filename) {
        return RegisterSample(filename)->GetHandle();
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

    void StartSound(int entityNum, const vec3_t origin, sfxHandle_t sfx) {
        AddSoundHelper(entityNum, origin, new OneShotSound(Sample::FromHandle(sfx)), 1);
    }

    void StartLocalSound(sfxHandle_t sfx) {
        AddSound(GetLocalEmitter(), new OneShotSound(Sample::FromHandle(sfx)), 1);
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

    void UpdateEntityPosition(int entityNum, const vec3_t position) {
        UpdateRegisteredEntityPosition(entityNum, position);
    }

    void UpdateEntityVelocity(int entityNum, const vec3_t velocity) {
        UpdateRegisteredEntityVelocity(entityNum, velocity);
    }

    void UpdateEntityOcclusion(int entityNum, float ratio) {
        //UpdateRegisteredEntityOcclusion(entityNum, ratio);
    }

}
