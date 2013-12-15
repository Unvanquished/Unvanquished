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

//TODO check all inputs for validity
namespace Audio {

    struct entityLoop_t {
        bool addedThisFrame;
        std::shared_ptr<LoopingSound> sound;
    };
    entityLoop_t entityLoops[MAX_GENTITIES];

    void Init() {
        InitSamples();
        InitSounds();
        InitEmitters();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            entityLoops[i] = {false, nullptr};
        }
    }

    void Shutdown() {
        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityLoops[i].sound) {
                entityLoops[i].sound->Stop();
            }
            entityLoops[i] = {false, nullptr};
        }

        ShutdownEmitters();
        ShutdownSounds();
        ShutdownSamples();
    }

    void Update() {
        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityLoops[i].sound and not entityLoops[i].addedThisFrame) {
                entityLoops[i].sound->FadeOutAndDie();
                entityLoops[i] = {false, nullptr};
            }
        }

        UpdateEmitters();
        UpdateSounds();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            entityLoops[i].addedThisFrame = false;
            if (entityLoops[i].sound.unique()) {
                entityLoops[i] = {false, nullptr};
            }
        }
    }

    sfxHandle_t RegisterSFX(Str::StringRef filename) {
        return RegisterSample(filename)->GetHandle();
    }

    void StartSound(int entityNum, const vec3_t origin, sfxHandle_t sfx) {
        std::shared_ptr<Emitter> emitter;

        // Apparently no origin means it is attached to an entity
        if (not origin) {
            emitter = GetEmitterForEntity(entityNum);

        } else {
            emitter = GetEmitterForPosition(origin);
        }

        AddSound(emitter, std::make_shared<OneShotSound>(Sample::FromHandle(sfx)), 1);
    }

    void StartLocalSound(sfxHandle_t sfx) {
        AddSound(GetLocalEmitter(), std::make_shared<OneShotSound>(Sample::FromHandle(sfx)), 1);
    }

    void AddEntityLoopingSound(int entityNum, sfxHandle_t sfx) {
        entityLoop_t& loop = entityLoops[entityNum];

        loop.addedThisFrame = true;

        if (not loop.sound) {
            loop.sound = std::make_shared<LoopingSound>(Sample::FromHandle(sfx));
            AddSound(GetEmitterForEntity(entityNum), loop.sound, 1);
        }
    }

    void ClearAllLoopingSounds() {
        for (int i = 0; i < MAX_GENTITIES; i++) {
            ClearLoopingSoundsForEntity(i);
        }
    }

    void ClearLoopingSoundsForEntity(int entityNum) {
        if (entityLoops[entityNum].sound) {
            entityLoops[entityNum].addedThisFrame = false;
        }
    }

    void UpdateEntityPosition(int entityNum, const vec3_t position) {
        UpdateRegisteredEntityPosition(entityNum, position);
    }

    void UpdateEntityVelocity(int entityNum, const vec3_t velocity) {
        UpdateRegisteredEntityVelocity(entityNum, velocity);
    }

    void UpdateEntityOcclusion(int entityNum, float ratio) {
        UpdateRegisteredEntityOcclusion(entityNum, ratio);
    }

}
