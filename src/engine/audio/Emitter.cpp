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

    // Structures to keep the state of entities we were given
    struct entityData_t {
        vec3_t position;
        vec3_t velocity;
        float occlusion;
    };
    static entityData_t entities[MAX_GENTITIES];
    static int listenerEntity = -1;

    // Keep Entitymitters in an array because there is at most one per entity.
    static std::shared_ptr<Emitter> entityEmitters[MAX_GENTITIES];

    // Position Emitters can be reused so we keep the list of all of them
    // this is not very efficient but we cannot have more position emitters
    // than sounds, that is about 128
    static std::vector<std::shared_ptr<PositionEmitter>> posEmitters;

    // There is a single LocalEmitter
    static std::shared_ptr<Emitter> localEmitter;

    static const vec3_t origin = {0.0f, 0.0f, 0.0f};

    static Cvar::Modified<Cvar::Cvar<bool>> useReverb("sound.reverb", "should reverb effects be used", Cvar::ARCHIVE, true);
    //TODO use optional once we have it
    static bool removeReverb = false;
    static bool addReverb = false;

    static AL::EffectSlot* globalEffect = nullptr;

    static bool initialized = false;

    void InitEmitters() {
        if (initialized) {
            return;
        }

        // This is a temporrary effect to test reverb
        AL::Effect effectParams;
        effectParams.ApplyReverbPreset(AL::GetHangarEffectPreset());

        globalEffect = new AL::EffectSlot();
        globalEffect->SetEffect(effectParams);

        AL::SetSpeedOfSound(SPEED_OF_SOUND);
        AL::SetDopplerExaggerationFactor(1); //keep it small else we get a deadlock in OpenAL's mixer

        localEmitter = std::make_shared<LocalEmitter>();

        initialized = true;
    }

    void ShutdownEmitters() {
        if (not initialized) {
            return;
        }

        localEmitter = nullptr;

        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityEmitters[i]) {
                entityEmitters[i] = nullptr;
            }
        }

        posEmitters.clear();

        delete globalEffect;
        globalEffect = nullptr;

        initialized = false;
    }

    void UpdateEmitters() {
        localEmitter->Update();

        // Both PositionEmitters and EntityEmitters are ref-counted.
        // If we hold the only reference to them then no sound is still using
        // the Emitter that can be destroyed.
        for (int i = 0; i < MAX_GENTITIES; i++) {
            auto emitter = entityEmitters[i];

            if (not emitter) {
                continue;
            }

            emitter->Update();

            // No sound is using this emitter, destroy it
            if (emitter.unique()) {
                entityEmitters[i] = nullptr;
            }
        }

        for (auto it = posEmitters.begin(); it != posEmitters.end();){
            (*it)->Update();

            // No sound is using this emitter, destroy it
            if ((*it).unique()) {
                it = posEmitters.erase(it);
            } else {
                it ++;
            }
        }

        addReverb = removeReverb = false;
        bool use;
        if (useReverb.GetModifiedValue(use)) {
            if (use) {
                addReverb = true;
            } else {
                removeReverb = true;
            }
        }
    }

    void UpdateListenerEntity(int entityNum, vec3_t orientation[3]) {
        listenerEntity = entityNum;

        AL::SetListenerPosition(entities[listenerEntity].position);
        AL::SetListenerVelocity(entities[listenerEntity].velocity);
        AL::SetListenerOrientation(orientation);
    }

    std::shared_ptr<Emitter> GetEmitterForEntity(int entityNum) {
        if (not entityEmitters[entityNum]) {
            entityEmitters[entityNum] = std::make_shared<EntityEmitter>(entityNum);
        }

        return entityEmitters[entityNum];
    }

    std::shared_ptr<Emitter> GetEmitterForPosition(const vec3_t position) {
        for (auto emitter : posEmitters) {
            if (Distance(emitter->GetPosition(), position) <= 1.0f) {
                return emitter;
            }
        }
        auto emitter = std::make_shared<PositionEmitter>(position);
        posEmitters.push_back(emitter);
        return emitter;
    }

    std::shared_ptr<Emitter> GetLocalEmitter() {
        return localEmitter;
    }

    void UpdateRegisteredEntityPosition(int entityNum, const vec3_t position) {
        VectorCopy(position, entities[entityNum].position);
    }

    void UpdateRegisteredEntityVelocity(int entityNum, const vec3_t velocity) {
        VectorCopy(velocity, entities[entityNum].velocity);
    }

    void UpdateRegisteredEntityOcclusion(int entityNum, float ratio) {
        entities[entityNum].occlusion = ratio;
    }

    // Utility functions for emitters

    // TODO avoid more unnecessary al calls

    void MakeLocal(AL::Source& source) {
        source.SetRelative(true);
        source.SetPosition(origin);
        source.SetVelocity(origin);

        source.DisableEffect(POSITIONAL_EFFECT_SLOT);
    }

    void Make3D(AL::Source& source, const vec3_t position, const vec3_t velocity, bool forceReverb = false) {
        source.SetRelative(false);
        source.SetPosition(position);
        source.SetVelocity(velocity);

        if ((forceReverb and useReverb.Get()) or addReverb) {
            source.EnableEffect(POSITIONAL_EFFECT_SLOT, *globalEffect);
        } else if ((forceReverb and not useReverb.Get()) or removeReverb) {
            source.DisableEffect(POSITIONAL_EFFECT_SLOT);
        }
    }

    // Implementation for Emitter

    Emitter::Emitter() {
    }

    Emitter::~Emitter() {
    }

    void Emitter::SetupSound(Sound& sound) {
        sound.GetSource().SetReferenceDistance(60.0f);
        InternalSetupSound(sound);
        UpdateSound(sound);
    }

    // Implementation of EntityEmitter

    EntityEmitter::EntityEmitter(int entityNum): entityNum(entityNum) {
    }

    EntityEmitter::~EntityEmitter(){
    }

    void EntityEmitter::Update() {
        // TODO
    }

    void EntityEmitter::UpdateSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

        if (entityNum == listenerEntity) {
            MakeLocal(source);
        } else {
            Make3D(source, entities[entityNum].position, entities[entityNum].velocity);
        }
    }

    void EntityEmitter::InternalSetupSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

        Make3D(source, entities[entityNum].position, entities[entityNum].velocity, true);
    }

    // Implementation of PositionEmitter

    PositionEmitter::PositionEmitter(const vec3_t position){
        VectorCopy(position, this->position);
    }

    PositionEmitter::~PositionEmitter() {
    }

    void PositionEmitter::Update() {
        //TODO
    }

    void PositionEmitter::UpdateSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

        Make3D(source, position, origin);
    }

    void PositionEmitter::InternalSetupSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

        Make3D(source, position, origin, true);
    }

    const vec3_t& PositionEmitter::GetPosition() const {
        return position;
    }

    // Implementation of LocalEmitter

    LocalEmitter::LocalEmitter() {
    }

    LocalEmitter::~LocalEmitter() {
    }

    void LocalEmitter::Update() {
    }

    void LocalEmitter::UpdateSound(Sound&) {
    }

    void LocalEmitter::InternalSetupSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

        MakeLocal(source);
    }

}
