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

#include "Emitter.h"

#include "../../common/Log.h"

namespace Audio {

    struct entityData_t {
        vec3_t position;
        vec3_t velocity;
        float occlusion;
    };

    static int listenerEntity = -1; //TODO

    static entityData_t entities[MAX_GENTITIES];

    static Emitter* entityEmitters[MAX_GENTITIES];
    static std::vector<PositionEmitter*> posEmitters;
    static Emitter* localEmitter;

    static const vec3_t origin = {0.0f, 0.0f, 0.0f};

    static bool initialized = false;

    void InitEmitters() {
        if (initialized) {
            return;
        }

        localEmitter = new LocalEmitter();

        initialized = true;
    }

    void ShutdownEmitters() {
        if (not initialized) {
            return;
        }

        delete localEmitter;

        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityEmitters[i]) {
                delete entityEmitters[i];
                entityEmitters[i] = nullptr;
            }
        }

        for (Emitter* emitter : posEmitters) {
            delete emitter;
        }
        posEmitters.clear();

        initialized = false;
    }

    void UpdateEverything() {
        localEmitter->Update();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            Emitter* emitter = entityEmitters[i];

            if (not emitter) {
                continue;
            }

            emitter->Update();
            if (not emitter->HasSounds()) {
                delete emitter;
                entityEmitters[i] = nullptr;
            }
        }

        for (auto it = posEmitters.begin(); it != posEmitters.end();){
            (*it)->Update();
            if ((*it)->HasSounds()) {
                it ++;
            } else {
                it = posEmitters.erase(it);
            }
        }
    }

    Emitter* GetEmitterForEntity(int entityNum) {
        if (not entityEmitters[entityNum]) {
            entityEmitters[entityNum] = new EntityEmitter(entityNum);
        }

        return entityEmitters[entityNum];
    }

    Emitter* GetEmitterForPosition(const vec3_t position) {
        for (PositionEmitter* emitter : posEmitters) {
            if (Distance(emitter->GetPosition(), position) <= 1.0f) {
                return emitter;
            }
        }
        PositionEmitter* emitter = new PositionEmitter(position);
        posEmitters.push_back(emitter);
        return emitter;
    }

    Emitter* GetLocalEmitter() {
        return localEmitter;
    }

    bool IsValidEntity(int entityNum) {
        return entityNum >= 0 and entityNum < MAX_GENTITIES;
    }

    void UpdateEntityPosition(int entityNum, const vec3_t position) {
        if (IsValidEntity(entityNum)) {
            VectorCopy(position, entities[entityNum].position);
        }
    }

    void UpdateEntityVelocity(int entityNum, const vec3_t velocity) {
        if (IsValidEntity(entityNum)) {
            VectorCopy(velocity, entities[entityNum].velocity);
        }
    }

    void UpdateEntityOcclusion(int entityNum, float ratio) {
        if (IsValidEntity(entityNum)) {
            entities[entityNum].occlusion = ratio;
        }
    }

    // Implementation for Emitter

    Emitter::Emitter() {
    }

    Emitter::~Emitter() {
        std::vector<Sound*> soundsBackup = this->sounds;
        for (Sound* sound: soundsBackup) {
            delete sound;
        }
    }

    void Emitter::Update() {
        std::vector<Sound*> soundsBackup = this->sounds;
        for (Sound* sound: soundsBackup) {
            sound->Update();
        }

        for (Sound* sound : sounds) {
            UpdateSource(sound->GetSource());
        }
    }

    void Emitter::SetupSource(AL::Source& source) {
        source.SetReferenceDistance(120.0f);
        InternalSetupSource(source);
        UpdateSource(source);
    }

    void Emitter::AddSound(Sound* sound) {
        sounds.push_back(sound);
        sound->SetEmitter(this);
    }

    void Emitter::RemoveSound(Sound* sound) {
        sounds.erase(std::remove(sounds.begin(), sounds.end(), sound), sounds.end());
    }

    bool Emitter::HasSounds() const {
        return not sounds.empty();
    }

    const std::vector<Sound*>& Emitter::GetSounds() {
        return sounds;
    }

    // Implementation of EntityEmitter

    EntityEmitter::EntityEmitter(int entityNum): entityNum(entityNum) {
    }

    EntityEmitter::~EntityEmitter(){
    }

    void EntityEmitter::UpdateSource(AL::Source& source) {
        if (entityNum == listenerEntity) {
            source.SetRelative(true);
            source.SetPosition(origin);
            source.SetVelocity(origin);

        } else {
            source.SetRelative(false);
            source.SetPosition(entities[entityNum].position);
            source.SetVelocity(entities[entityNum].velocity);

        }
    }

    void EntityEmitter::InternalSetupSource(AL::Source& source) {
    }

    // Implementation of PositionEmitter

    PositionEmitter::PositionEmitter(const vec3_t positon){
        VectorCopy(position, this->position);
    }

    PositionEmitter::~PositionEmitter() {
    }

    void PositionEmitter::UpdateSource(AL::Source& source) {
    }

    void PositionEmitter::InternalSetupSource(AL::Source& source) {
        source.SetRelative(false);
        source.SetPosition(position);
        source.SetVelocity(origin);
    }

    const vec3_t& PositionEmitter::GetPosition() const {
        return position;
    }

    // Implementation of LocalEmitter

    LocalEmitter::LocalEmitter() {
    }

    LocalEmitter::~LocalEmitter() {
    }

    void LocalEmitter::UpdateSource(AL::Source& source) {
    }

    void LocalEmitter::InternalSetupSource(AL::Source& source) {
        source.SetRelative(true);
        source.SetPosition(origin);
        source.SetVelocity(origin);
    }

}
