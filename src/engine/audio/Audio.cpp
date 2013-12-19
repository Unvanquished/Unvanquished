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

    static Cvar::Cvar<float> masterVolume("sound.volume.master", "the global audio volume", Cvar::ARCHIVE, 0.8f);

    //TODO make them the equivalent of LATCH and ROM for available*
    static Cvar::Cvar<std::string> deviceString("s_alDevice", "the OpenAL device to use", Cvar::ARCHIVE, "");
    static Cvar::Cvar<std::string> availableDevices("s_alAvailableDevices", "the available OpenAL devices", 0, "");

    static Cvar::Cvar<std::string> inputDeviceString("s_alInputDevice", "the OpenAL input device to use", Cvar::ARCHIVE, "");
    static Cvar::Cvar<std::string> availableInputDevices("s_alAvailableInputDevices", "the available input OpenAL devices", 0, "");

    struct entityLoop_t {
        bool addedThisFrame;
        std::shared_ptr<LoopingSound> sound;
    };
    static entityLoop_t entityLoops[MAX_GENTITIES];

    static bool initialized = false;
    static AL::Device* device;
    static AL::Context* context;

    std::shared_ptr<MusicSound> music;

    bool Init() {
        if (initialized) {
            return true;
        }

        std::string deviceToTry = deviceString.Get();
        if (not deviceToTry.empty()) {
            device = AL::Device::FromName(deviceToTry);

            if (not device) {
                Log::Warn("Could not open OpenAL device '%s', trying the default device.", deviceToTry);
                device = AL::Device::GetDefaultDevice();
            }
        } else {
            device = AL::Device::GetDefaultDevice();
        }

        if (not device) {
            Log::Warn("Could not open an OpenAL device.");
            return false;
        }

        context = AL::Context::GetDefaultContext(*device);

        if (not context) {
            delete device;
            device = nullptr;
            Log::Warn("Could not create an OpenAL context.");
            return false;
        }

        std::stringstream deviceList;
        for (auto deviceName : AL::Device::ListByName()) {
            deviceList << deviceName << "\n";
        }
        availableDevices.Set(deviceList.str());
        Log::Debug(deviceList.str());

        context->MakeCurrent();

        initialized = true;

        InitSamples();
        InitSounds();
        InitEmitters();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            entityLoops[i] = {false, nullptr};
        }

        return true;
    }

    void Shutdown() {
        if (not initialized) {
            return;
        }
        initialized = false;

        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityLoops[i].sound) {
                entityLoops[i].sound->Stop();
            }
            entityLoops[i] = {false, nullptr};
        }

        music = nullptr;

        ShutdownEmitters();
        ShutdownSounds();
        ShutdownSamples();

        delete context;
        delete device;

        context = nullptr;
        device = nullptr;
    }

    void Update() {
        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityLoops[i].sound and not entityLoops[i].addedThisFrame) {
                entityLoops[i].sound->FadeOutAndDie();
                entityLoops[i] = {false, nullptr};
            }
        }

        AL::SetListenerGain(masterVolume.Get());

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

    void StartMusic(Str::StringRef leadingSound, Str::StringRef loopSound) {
        StopMusic();
        music = std::make_shared<MusicSound>(leadingSound, loopSound);
        AddSound(GetLocalEmitter(), music, 1);
    }

    void StopMusic() {
        if (music) {
            music->Stop();
        }
        music = nullptr;
    }

    void UpdateListener(int entityNum, vec3_t orientation[3]) {
        UpdateListenerEntity(entityNum, orientation);
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
