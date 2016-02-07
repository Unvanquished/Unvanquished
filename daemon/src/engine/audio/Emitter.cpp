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

    // Structures to keep the state of entities we were given
    struct entityData_t {
        Vec3 position;
        Vec3 velocity;
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

    static const Vec3 origin = {0.0f, 0.0f, 0.0f};

    static Cvar::Range<Cvar::Cvar<float>> dopplerExaggeration("audio.dopplerExaggeration", "controls the pitch change of the doppler effect", Cvar::ARCHIVE, 0.4, 0.0, 1.0);
    static Cvar::Range<Cvar::Cvar<float>> reverbIntensity("audio.reverbIntensity", "the intensity of the reverb effects", Cvar::ARCHIVE, 1.0, 0.0, 1.0);

    struct ReverbSlot {
        AL::EffectSlot* effect;
        std::string name;
        float ratio;
        float askedRatio;
    };
    ReverbSlot reverbSlots[N_REVERB_SLOTS];
    bool testingReverb = false;

    static bool initialized = false;

    void InitEmitters() {
        if (initialized) {
            return;
        }

        AL::Effect effectParams;
        effectParams.ApplyReverbPreset(*AL::GetPresetByName("generic"));

        for (int i = 0; i < N_REVERB_SLOTS; i++) {
            reverbSlots[i].effect = new AL::EffectSlot();
            reverbSlots[i].effect->SetEffect(effectParams);
            reverbSlots[i].effect->SetGain(1.0f);
            reverbSlots[i].name = "generic";
            reverbSlots[i].ratio = 1.0f;
            reverbSlots[i].askedRatio = 1.0f;
        }

        AL::SetInverseDistanceModel();
        AL::SetSpeedOfSound(SPEED_OF_SOUND);

        localEmitter = std::make_shared<LocalEmitter>();

        initialized = true;
    }

    void ShutdownEmitters() {
        if (not initialized) {
            return;
        }

        localEmitter = nullptr;

        for (int i = 0; i < N_REVERB_SLOTS; i++) {
            delete reverbSlots[i].effect;
            reverbSlots[i].effect = nullptr;
        }

        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityEmitters[i]) {
                entityEmitters[i] = nullptr;
            }
        }

        posEmitters.clear();

        testingReverb = false;

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

        float reverbVolume = reverbIntensity.Get();
        for (int i = 0; i < N_REVERB_SLOTS; i++) {
            reverbSlots[i].effect->SetGain(reverbSlots[i].ratio * reverbVolume);
        }

        AL::SetDopplerExaggerationFactor(dopplerExaggeration.Get());
    }

    void UpdateListenerEntity(int entityNum, const Vec3 orientation[3]) {
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

    std::shared_ptr<Emitter> GetEmitterForPosition(Vec3 position) {
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

    void UpdateRegisteredEntityPosition(int entityNum, Vec3 position) {
        VectorCopy(position, entities[entityNum].position);
    }

    void UpdateRegisteredEntityVelocity(int entityNum, Vec3 velocity) {
        VectorCopy(velocity, entities[entityNum].velocity);
    }

    void UpdateReverbSlot(int slotNum, std::string name, float ratio) {
        ASSERT_GE(slotNum, 0);
        ASSERT_LT(slotNum, N_REVERB_SLOTS);
        ASSERT(!std::isnan(ratio));

        auto& slot = reverbSlots[slotNum];

        if (not testingReverb and name == "none") {
            ratio = 0.0f;
            name = slot.name;
        }

        if (name != slot.name) {
            if (not testingReverb) {
                auto preset = AL::GetPresetByName(name);

                if (not preset) {
                    audioLogs.Warn("Reverb preset '%s' doesn't exist.", name);
                    return;
                }

                AL::Effect effectParams;
                effectParams.ApplyReverbPreset(*preset);
                slot.effect->SetEffect(effectParams);
            }

            slot.name = std::move(name);
        }

        if (not testingReverb) {
            slot.ratio = ratio;
        }
        slot.askedRatio = ratio;
    }

    // Utility functions for emitters

    // TODO avoid more unnecessary al calls

    void MakeLocal(AL::Source& source) {
        source.SetRelative(true);
        source.SetPosition(origin);
        source.SetVelocity(origin);

        for (int i = 0; i < N_REVERB_SLOTS; i++) {
            source.DisableEffect(i);
        }
    }

    void Make3D(AL::Source& source, Vec3 position, Vec3 velocity) {
        source.SetRelative(false);
        source.SetPosition(position);
        source.SetVelocity(velocity);

        for (int i = 0; i < N_REVERB_SLOTS; i++) {
            source.EnableEffect(i, *reverbSlots[i].effect);
        }
    }

    // Implementation for Emitter

    Emitter::Emitter() {
    }

    Emitter::~Emitter() {
    }

    void Emitter::SetupSound(Sound& sound) {
        sound.GetSource().SetReferenceDistance(120.0f);
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

        Make3D(source, entities[entityNum].position, entities[entityNum].velocity);
    }

    // Implementation of PositionEmitter

    PositionEmitter::PositionEmitter(Vec3 position){
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

        Make3D(source, position, origin);
    }

    Vec3 PositionEmitter::GetPosition() const {
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

    class TestReverbCmd : public Cmd::StaticCmd {
        public:
            TestReverbCmd(): StaticCmd("testReverb", Cmd::AUDIO, "Tests a reverb preset.") {
            }

            virtual void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() != 2) {
                    PrintUsage(args, "stop", "stop the test");
                    PrintUsage(args, "[preset name]", "tests the reverb preset.");
                    return;
                } else {
                    if (args.Argv(1) == "stop") {
                        StopTest();
                    } else {
                        StartTest(args);
                    }
                }
            }

            Cmd::CompletionResult Complete(int argNum, const Cmd::Args&, Str::StringRef prefix) const OVERRIDE {
                if (argNum != 1) {
                    return {};
                }
                std::vector<std::string> presets = AL::ListPresetNames();
                std::sort(presets.begin(), presets.end());

                Cmd::CompletionResult res;

                for (auto& name: presets) {
                    if (Str::IsPrefix(prefix, name)) {
                        res.push_back({std::move(name), ""});
                    }
                }

                Cmd::AddToCompletion(res, prefix, {{"stop", "stops the test"}, {"none", "no effect at all"}});

                return res;
            }

        private:
            void StartTest(const Cmd::Args& args) const {
                const std::string& name = args.Argv(1);

                if (name == "none") {
                    testingReverb = true;
                    for (int i = 0; i < N_REVERB_SLOTS; i++) {
                        reverbSlots[i].effect->SetGain(0.0f);
                    }
                    return;
                }

                auto preset = AL::GetPresetByName(name);

                if (not preset) {
                    Print("Reverb preset '%s' doesn't exist.", name);
                    return;
                }

                AL::Effect effectParams;
                effectParams.ApplyReverbPreset(*preset);

                testingReverb = true;
                for (int i = 1; i < N_REVERB_SLOTS; i++) {
                    reverbSlots[i].effect->SetGain(0.0f);
                }

                reverbSlots[0].ratio = 0.5f;
                reverbSlots[0].effect->SetEffect(effectParams);
            }

            void StopTest() const {
                testingReverb = false;
                for (int i = 0; i < N_REVERB_SLOTS; i++) {
                    std::string name = std::move(reverbSlots[i].name);
                    UpdateReverbSlot(i, std::move(name), reverbSlots[i].askedRatio);
                }
            }
    };
    static TestReverbCmd testReverbRegistration;

}
