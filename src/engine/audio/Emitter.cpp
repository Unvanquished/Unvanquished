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

#include "ALObjects.h"
#include "Emitter.h"
#include "Sound.h"

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

    void UpdateEmitters() {
        localEmitter->Update();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            Emitter* emitter = entityEmitters[i];

            if (not emitter) {
                continue;
            }

            emitter->Update();
            if (emitter->GetRefCount() == 0) {
                delete emitter;
                entityEmitters[i] = nullptr;
            }
        }

        for (auto it = posEmitters.begin(); it != posEmitters.end();){
            (*it)->Update();
            if ((*it)->GetRefCount() != 0) {
                it ++;
            } else {
                delete (*it);
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

    void UpdateRegisteredEntityPosition(int entityNum, const vec3_t position) {
        VectorCopy(position, entities[entityNum].position);
    }

    void UpdateRegisteredEntityVelocity(int entityNum, const vec3_t velocity) {
        VectorCopy(velocity, entities[entityNum].velocity);
    }

    void UpdateRegisteredEntityOcclusion(int entityNum, float ratio) {
        entities[entityNum].occlusion = ratio;
    }

    // Implementation for Emitter

    Emitter::Emitter() : refCount(0) {
    }

    Emitter::~Emitter() {
    }

    void Emitter::SetupSound(Sound& sound) {
        sound.GetSource().SetReferenceDistance(60.0f);
        InternalSetupSound(sound);
        UpdateSound(sound);
    }

    void Emitter::Retain() {
        refCount ++;
    }

    void Emitter::Release() {
        refCount --;
    }

    int Emitter::GetRefCount() {
        return refCount;
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
            source.SetRelative(true);
            source.SetPosition(origin);
            source.SetVelocity(origin);

        } else {
            source.SetRelative(false);
            source.SetPosition(entities[entityNum].position);
            source.SetVelocity(entities[entityNum].velocity);

        }
    }

    void EntityEmitter::InternalSetupSound(Sound& sound) {
    }

    // Implementation of PositionEmitter

    PositionEmitter::PositionEmitter(const vec3_t positon){
        VectorCopy(position, this->position);
    }

    PositionEmitter::~PositionEmitter() {
    }

    void PositionEmitter::Update() {
        //TODO
    }

    void PositionEmitter::UpdateSound(Sound& sound) {
    }

    void PositionEmitter::InternalSetupSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

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

    void LocalEmitter::Update() {
    }

    void LocalEmitter::UpdateSound(Sound& sound) {
    }

    void LocalEmitter::InternalSetupSound(Sound& sound) {
        AL::Source& source = sound.GetSource();

        source.SetRelative(true);
        source.SetPosition(origin);
        source.SetVelocity(origin);
    }

}
