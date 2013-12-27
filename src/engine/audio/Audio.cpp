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

    Log::Logger audioLogs("audio");

    static Cvar::Cvar<float> masterVolume("audio.volume.master", "the global audio volume", Cvar::ARCHIVE, 0.8f);

    static Cvar::Cvar<bool> muteWhenMinimized("audio.muteWhenMinimized", "should the game be muted when minimized", Cvar::ARCHIVE, false);
    static Cvar::Cvar<bool> muteWhenUnfocused("audio.muteWhenUnfocused", "should the game be muted when not focused", Cvar::ARCHIVE, false);

    //TODO make them the equivalent of LATCH and ROM for available*
    static Cvar::Cvar<std::string> deviceString("audio.al.device", "the OpenAL device to use", Cvar::ARCHIVE, "");
    static Cvar::Cvar<std::string> availableDevices("audio.al.availableDevices", "the available OpenAL devices", 0, "");

    static Cvar::Cvar<std::string> captureDeviceString("audio.al.captureDevice", "the OpenAL capture device to use", Cvar::ARCHIVE, "");
    static Cvar::Cvar<std::string> availableCaptureDevices("audio.al.availableCaptureDevices", "the available capture OpenAL devices", 0, "");

    // We mimic the behavior of the previous sound system by allowing only one looping ousnd per entity.
    // (and only one entities) CGame will add at each frame all the loops: if a loop hasn't been given
    // in a frame, it means it sould be destroyed.
    struct entityLoop_t {
        bool addedThisFrame;
        std::shared_ptr<LoopingSound> sound;
        sfxHandle_t newSfx;
        sfxHandle_t oldSfx;
    };
    static entityLoop_t entityLoops[MAX_GENTITIES];

    static std::shared_ptr<StreamingSound> streams[N_STREAMS];

    static bool initialized = false;

    static AL::Device* device;
    static AL::Context* context;

    void CaptureTestStop();
    void CaptureTestUpdate();

    // Like in the previous sound system, we only have a single music
    std::shared_ptr<MusicSound> music;

    bool IsValidEntity(int entityNum) {
        return entityNum >= 0 and entityNum < MAX_GENTITIES;
    }

    bool IsValidVector(const vec3_t v) {
        return not std::isnan(v[0]) and not std::isnan(v[1]) and not std::isnan(v[2]);
    }

    bool Init() {
        if (initialized) {
            return true;
        }

        // Initializes a device
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

        // Initializes a context
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

        context->MakeCurrent();

        // Initializes the list of input devices

        std::stringstream captureDeviceList;
        for (auto captureDeviceName : AL::CaptureDevice::ListByName()) {
            captureDeviceList << captureDeviceName << "\n";
        }
        availableCaptureDevices.Set(captureDeviceList.str());

        audioLogs.Notice(AL::GetSystemInfo(device, nullptr));

        initialized = true;

        // Initializes the rest of the audio system
        AL::InitEffectPresets();
        InitSamples();
        InitSounds();
        InitEmitters();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            entityLoops[i] = {false, nullptr, -1, -1};
        }

        return true;
    }

    void Shutdown() {
        if (not initialized) {
            return;
        }

        // Shuts down the wrapper
        for (int i = 0; i < MAX_GENTITIES; i++) {
            if (entityLoops[i].sound) {
                entityLoops[i].sound->Stop();
            }
            entityLoops[i] = {false, nullptr, -1, -1};
        }

        StopMusic();
        CaptureTestStop();
        StopCapture();

        // Shuts down the rest of the system
        // Shutdown sounds before emitters because they are using a effects defined in emitters
        // and that OpenAL checks the refcount before deleting the object.
        ShutdownSounds();
        ShutdownEmitters();
        ShutdownSamples();

        // Free OpenAL resources
        delete context;
        delete device;

        context = nullptr;
        device = nullptr;

        initialized = false;
    }

    void Update() {
        if (not initialized) {
            return;
        }

        for (int i = 0; i < MAX_GENTITIES; i++) {
            auto& loop = entityLoops[i];
            if (loop.sound and not loop.addedThisFrame) {
                // The loop wasn't added this frame, that means it has to be removed.
                loop.sound->FadeOutAndDie();
                loop = {false, nullptr, -1, -1};

            } else if (loop.oldSfx != loop.newSfx) {
                // The last sfx added in the frame is not the current one being played
                // To mimic the previous sound system's behavior we sart playing the new one.
                loop.sound->FadeOutAndDie();

                int newSfx = loop.newSfx;
                loop = {false, nullptr, -1, -1};

                AddEntityLoopingSound(i, newSfx);
            }
        }

        if ((muteWhenMinimized.Get() and com_minimized->integer) or (muteWhenUnfocused.Get() and com_unfocused->integer)) {
            AL::SetListenerGain(0.0f);
        } else {
            AL::SetListenerGain(masterVolume.Get());
        }

        // Update the rest of the system
        CaptureTestUpdate();
        UpdateEmitters();
        UpdateSounds();

        for (int i = 0; i < MAX_GENTITIES; i++) {
            entityLoops[i].addedThisFrame = false;
            // if we are the unique owner of a loop pointer, then it means it was stopped, free it.
            if (entityLoops[i].sound.unique()) {
                entityLoops[i] = {false, nullptr, -1, -1};
            }
        }

        for (int i = 0; i < N_STREAMS; i++) {
            if (streams[i] and streams[i].unique()) {
                streams[i] = nullptr;
            }
        }
    }

    sfxHandle_t RegisterSFX(Str::StringRef filename) {
        // TODO: what should we do if we aren't initialized?
        return RegisterSample(filename)->GetHandle();
    }

    void StartSound(int entityNum, const vec3_t origin, sfxHandle_t sfx) {
        if (not initialized or not Sample::IsValidHandle(sfx)) {
            return;
        }

        std::shared_ptr<Emitter> emitter;

        // Apparently no origin means it is attached to an entity
        if (not origin and IsValidEntity(entityNum)) {
            emitter = GetEmitterForEntity(entityNum);

        } else if(origin and IsValidVector(origin)){
            emitter = GetEmitterForPosition(origin);

        } else {
            return;
        }

        AddSound(emitter, std::make_shared<OneShotSound>(Sample::FromHandle(sfx)), 1);
    }

    void StartLocalSound(sfxHandle_t sfx) {
        if (not initialized or not Sample::IsValidHandle(sfx)) {
            return;
        }

        AddSound(GetLocalEmitter(), std::make_shared<OneShotSound>(Sample::FromHandle(sfx)), 1);
    }

    void AddEntityLoopingSound(int entityNum, sfxHandle_t sfx) {
        if (not initialized or not Sample::IsValidHandle(sfx) or not IsValidEntity(entityNum)) {
            return;
        }

        entityLoop_t& loop = entityLoops[entityNum];

        // If we have no sound we can play the loop directly
        if (not loop.sound) {
            loop.sound = std::make_shared<LoopingSound>(Sample::FromHandle(sfx));
            loop.oldSfx = sfx;
            AddSound(GetEmitterForEntity(entityNum), loop.sound, 1);
        }
        loop.addedThisFrame = true;

        // We remember what is the last sfx asked because cgame expects the sfx added last in the frame to be played
        loop.newSfx = sfx;
    }

    void ClearAllLoopingSounds() {
        if (not initialized) {
            return;
        }

        for (int i = 0; i < MAX_GENTITIES; i++) {
            ClearLoopingSoundsForEntity(i);
        }
    }

    void ClearLoopingSoundsForEntity(int entityNum) {
        if (not initialized or not IsValidEntity(entityNum)) {
            return;
        }

        if (entityLoops[entityNum].sound) {
            entityLoops[entityNum].addedThisFrame = false;
        }
    }

    void StartMusic(Str::StringRef leadingSound, Str::StringRef loopSound) {
        if (not initialized) {
            return;
        }

        StopMusic();
        music = std::make_shared<MusicSound>(leadingSound, loopSound);
        AddSound(GetLocalEmitter(), music, 1);
    }

    void StopMusic() {
        if (not initialized) {
            return;
        }

        if (music) {
            music->Stop();
        }
        music = nullptr;
    }

    void StreamData(int streamNum, const void* data, int numSamples, int rate, int width, float volume, int entityNum) {
        if (not initialized or (streamNum < 0 or streamNum >= N_STREAMS)) {
            return;
        }

        if (not streams[streamNum]) {
            streams[streamNum] = std::make_shared<StreamingSound>();
            if (IsValidEntity(entityNum)) {
                AddSound(GetLocalEmitter(), streams[streamNum], 1);
            } else {
                AddSound(GetEmitterForEntity(entityNum), streams[streamNum], 1);
            }
        }

        streams[streamNum]->SetGain(volume);

        snd_info_t dataInfo = {rate, width, 1, numSamples, (width * numSamples), 0};
        AL::Buffer buffer;

        int feedError = buffer.Feed(dataInfo, data);

        if (not feedError) {
            streams[streamNum]->AppendBuffer(std::move(buffer));
        }
    }

    void UpdateListener(int entityNum, const vec3_t orientation[3]) {
        if (not initialized or
            not IsValidEntity(entityNum) or
            not orientation or
            not IsValidVector(orientation[0]) or
            not IsValidVector(orientation[1]) or
            not IsValidVector(orientation[2])) {
            return;
        }

        UpdateListenerEntity(entityNum, orientation);
    }

    void UpdateEntityPosition(int entityNum, const vec3_t position) {
        if (not initialized or not IsValidEntity(entityNum) or not IsValidVector(position)) {
            return;
        }

        UpdateRegisteredEntityPosition(entityNum, position);
    }

    void UpdateEntityVelocity(int entityNum, const vec3_t velocity) {
        if (not initialized or not IsValidEntity(entityNum) or not IsValidVector(velocity)) {
            return;
        }

        UpdateRegisteredEntityVelocity(entityNum, velocity);
    }

    // Capture functions

    static AL::CaptureDevice* capture = nullptr;

    void StartCapture(int rate) {
        if (capture or not initialized) {
            return;
        }

        audioLogs.Notice("Started the OpenAL capture with rate %i", rate);
        capture = AL::CaptureDevice::GetDefaultDevice(rate);
        capture->Start();
    }

    int AvailableCaptureSamples() {
        if (not capture) {
            return 0;
        }

        return capture->GetNumCapturedSamples();
    }

    void GetCapturedData(int numSamples, void* buffer) {
        if (not capture) {
            return;
        }

        capture->Capture(numSamples, buffer);
    }

    void StopCapture() {
        if (not initialized) {
            return;
        }

        if (capture) {
            audioLogs.Notice("Stopped the OpenAL capture");
            capture->Stop();
        }
        delete capture;
        capture = nullptr;
    }

    bool doingCaptureTest = false;

    void CaptureTestStart() {
        if (not initialized or capture or doingCaptureTest) {
            return;
        }

        audioLogs.Notice("Starting the sound capture test");
        StartCapture(16000);
        doingCaptureTest = true;
    }

    void CaptureTestUpdate() {
        if (not doingCaptureTest) {
            return;
        }

        int numSamples = AvailableCaptureSamples();

        if (numSamples > 0) {
            uint16_t* buffer = new uint16_t[numSamples];
            GetCapturedData(numSamples, buffer);
            StreamData(N_STREAMS - 1, buffer, numSamples, 16000, 2, 1.0, -1);
            delete[] buffer;
        }
    }

    void CaptureTestStop() {
        if (not doingCaptureTest) {
            return;
        }

        audioLogs.Notice("Stopping the sound capture test");
        StopCapture();
        doingCaptureTest = false;
    }

    // Console commands

    class ALInfoCmd : public Cmd::StaticCmd {
        public:
            ALInfoCmd(): StaticCmd("printALInfo", Cmd::AUDIO, N_("Prints information about OpenAL")) {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                Print(AL::GetSystemInfo(device, capture));
            }
    };
    static ALInfoCmd alInfoRegistration;

    class ListSamplesCmd : public Cmd::StaticCmd {
        public:
            ListSamplesCmd(): StaticCmd("listAudioSamples", Cmd::AUDIO, N_("Lists all the loaded sound samples")) {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                std::vector<std::string> samples = ListSamples();

                std::sort(samples.begin(), samples.end());

                for (auto& sample: samples) {
                    Print(sample);
                }
                Print("%i samples", samples.size());
            }
    };
    static ListSamplesCmd listSamplesRegistration;

    class StopSoundsCmd : public Cmd::StaticCmd {
        public:
            StopSoundsCmd(): StaticCmd("stopSounds", Cmd::AUDIO, N_("Stops the music and the looping sounds")) {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                ClearAllLoopingSounds();
                StopMusic();
            }
    };
    static StopSoundsCmd stopSoundsRegistration;

    class StartCaptureTestCmd : public Cmd::StaticCmd {
        public:
            StartCaptureTestCmd(): StaticCmd("startSoundCaptureTest", Cmd::AUDIO, N_("Starts testing the sound capture")) {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                CaptureTestStart();
            }
    };
    static StartCaptureTestCmd startCaptureTestRegistration;

    class StopCaptureTestCmd : public Cmd::StaticCmd {
        public:
            StopCaptureTestCmd(): StaticCmd("stopSoundCaptureTest", Cmd::AUDIO, N_("Stops the testing of the sound capture")) {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                CaptureTestStop();
            }
    };
    static StopCaptureTestCmd stopCaptureTestRegistration;
}
