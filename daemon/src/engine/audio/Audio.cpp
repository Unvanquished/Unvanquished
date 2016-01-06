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
#include "AudioData.h"

namespace Audio {

    Log::Logger audioLogs("audio");

    static Cvar::Range<Cvar::Cvar<float>> masterVolume("audio.volume.master", "the global audio volume", Cvar::ARCHIVE, 0.8f, 0.0f, 1.0f);

    static Cvar::Cvar<bool> muteWhenMinimized("audio.muteWhenMinimized", "should the game be muted when minimized", Cvar::NONE, false);
    static Cvar::Cvar<bool> muteWhenUnfocused("audio.muteWhenUnfocused", "should the game be muted when not focused", Cvar::NONE, false);

    //TODO make them the equivalent of LATCH and ROM for available*
    static Cvar::Cvar<std::string> deviceString("audio.al.device", "the OpenAL device to use", Cvar::ARCHIVE, "");
    static Cvar::Cvar<std::string> availableDevices("audio.al.availableDevices", "the available OpenAL devices", Cvar::NONE, "");

    static Cvar::Cvar<std::string> captureDeviceString("audio.al.captureDevice", "the OpenAL capture device to use", Cvar::ARCHIVE, "");
    static Cvar::Cvar<std::string> availableCaptureDevices("audio.al.availableCaptureDevices", "the available capture OpenAL devices", Cvar::NONE, "");

    // We mimic the behavior of the previous sound system by allowing only one looping sound per entity.
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
    std::shared_ptr<LoopingSound> music;

    bool IsValidEntity(int entityNum) {
        return entityNum >= 0 and entityNum < MAX_GENTITIES;
    }

    bool IsValidVector(Vec3 v) {
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
        for (const auto& deviceName : AL::Device::ListByName()) {
            deviceList << deviceName << "\n";
        }
        availableDevices.Set(deviceList.str());

        context->MakeCurrent();

        // Initializes the list of input devices

        std::stringstream captureDeviceList;
        for (const auto& captureDeviceName : AL::CaptureDevice::ListByName()) {
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

        UpdateListenerGain();

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

        UpdateListenerGain();

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

    void BeginRegistration() {
        BeginSampleRegistration();
    }

    sfxHandle_t RegisterSFX(Str::StringRef filename) {
        // TODO: what should we do if we aren't initialized?
        return RegisterSample(filename)->GetHandle();
    }

    void EndRegistration() {
        EndSampleRegistration();
    }

    void StartSound(int entityNum, Vec3 origin, sfxHandle_t sfx) {
        if (not initialized or not Sample::IsValidHandle(sfx)) {
            return;
        }

        std::shared_ptr<Emitter> emitter;

        // A valid number means it is an entity sound
        if (IsValidEntity(entityNum)) {
            emitter = GetEmitterForEntity(entityNum);

        } else if(IsValidVector(origin)){
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

        std::shared_ptr<Sample> leadingSample = nullptr;
        std::shared_ptr<Sample> loopingSample = nullptr;
        if (not leadingSound.empty()) {
            leadingSample = RegisterSample(leadingSound);
        }
        if (not loopSound.empty()) {
            loopingSample = RegisterSample(loopSound);
        }

        StopMusic();
        music = std::make_shared<LoopingSound>(loopingSample, leadingSample);
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

    void StopAllSounds() {
        StopMusic();
        StopSounds();
    }

    void StreamData(int streamNum, const void* data, int numSamples, int rate, int width, int channels, float volume, int entityNum) {
        if (not initialized or (streamNum < 0 or streamNum >= N_STREAMS)) {
            return;
        }

        if (not streams[streamNum]) {
            streams[streamNum] = std::make_shared<StreamingSound>();
            if (IsValidEntity(entityNum)) {
                AddSound(GetEmitterForEntity(entityNum), streams[streamNum], 1);
            } else {
                AddSound(GetLocalEmitter(), streams[streamNum], 1);
            }
        }

        streams[streamNum]->SetGain(volume);

	    AudioData audioData(rate, width, channels, (width * numSamples * channels),
	                        reinterpret_cast<const char*>(data));
	    AL::Buffer buffer;

	    int feedError = buffer.Feed(audioData);

        if (not feedError) {
            streams[streamNum]->AppendBuffer(std::move(buffer));
        }
    }

    void UpdateListener(int entityNum, const Vec3 orientation[3]) {
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

    void UpdateListenerGain() {
        if ((muteWhenMinimized.Get() and com_minimized->integer) or (muteWhenUnfocused.Get() and com_unfocused->integer)) {
            AL::SetListenerGain(0.0f);
        } else {
            AL::SetListenerGain(SliderToAmplitude(masterVolume.Get()));
        }
    }

    void UpdateEntityPosition(int entityNum, Vec3 position) {
        if (not initialized or not IsValidEntity(entityNum) or not IsValidVector(position)) {
            return;
        }

        UpdateRegisteredEntityPosition(entityNum, position);
    }

    void UpdateEntityVelocity(int entityNum, Vec3 velocity) {
        if (not initialized or not IsValidEntity(entityNum) or not IsValidVector(velocity)) {
            return;
        }

        UpdateRegisteredEntityVelocity(entityNum, velocity);
    }

    void SetReverb(int slotNum, std::string name, float ratio) {
        if (slotNum < 0 or slotNum >= N_REVERB_SLOTS or std::isnan(ratio)) {
            return;
        }

        UpdateReverbSlot(slotNum, std::move(name), ratio);
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
            StreamData(N_STREAMS - 1, buffer, numSamples, 16000, 2, 1, 1.0, -1);
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
            ALInfoCmd(): StaticCmd("printALInfo", Cmd::AUDIO, "Prints information about OpenAL") {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                Print(AL::GetSystemInfo(device, capture));
            }
    };
    static ALInfoCmd alInfoRegistration;

    class ListSamplesCmd : public Cmd::StaticCmd {
        public:
            ListSamplesCmd(): StaticCmd("listAudioSamples", Cmd::AUDIO, "Lists all the loaded sound samples") {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                std::vector<std::string> samples = ListSamples();

                std::sort(samples.begin(), samples.end());

                for (const auto& sample: samples) {
                    Print(sample);
                }
                Print("%i samples", samples.size());
            }
    };
    static ListSamplesCmd listSamplesRegistration;

    class StopSoundsCmd : public Cmd::StaticCmd {
        public:
            StopSoundsCmd(): StaticCmd("stopSounds", Cmd::AUDIO, "Stops the music and the looping sounds") {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                ClearAllLoopingSounds();
                StopMusic();
            }
    };
    static StopSoundsCmd stopSoundsRegistration;

    class StartCaptureTestCmd : public Cmd::StaticCmd {
        public:
            StartCaptureTestCmd(): StaticCmd("startSoundCaptureTest", Cmd::AUDIO, "Starts testing the sound capture") {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                CaptureTestStart();
            }
    };
    static StartCaptureTestCmd startCaptureTestRegistration;

    class StopCaptureTestCmd : public Cmd::StaticCmd {
        public:
            StopCaptureTestCmd(): StaticCmd("stopSoundCaptureTest", Cmd::AUDIO, "Stops the testing of the sound capture") {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                CaptureTestStop();
            }
    };
    static StopCaptureTestCmd stopCaptureTestRegistration;

    // Play the sounds from the filenames specified as arguments of the command
    class PlaySoundCmd : public Cmd::StaticCmd {
        public:
            PlaySoundCmd(): StaticCmd("playSound", Cmd::AUDIO, "Plays the given sound effects") {
            }

            virtual void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() == 1) {
                    PrintUsage(args, "/playSound <file> [<file>…]", "play sound files");
                    return;
                }

                for (int i = 1; i < args.Argc(); i++) {
                    sfxHandle_t soundHandle = RegisterSFX(args.Argv(i));
                    StartLocalSound(soundHandle);
                }
            }

            virtual Cmd::CompletionResult Complete(int argNum, const Cmd::Args&, Str::StringRef prefix) const OVERRIDE {
                if (argNum >= 1) {
                    //TODO have a list of supported extensions somewhere and use that?
                    return FS::PakPath::CompleteFilename(prefix, "", "", true, false);
                }

                return {};
            }
    };
    static PlaySoundCmd playSoundRegistration;

    // Play the music from the filename specified as argument of the command
    class PlayMusicCmd : public Cmd::StaticCmd {
        public:
            PlayMusicCmd(): StaticCmd("playMusic", Cmd::AUDIO, "Plays a music") {
            }

            virtual void Run(const Cmd::Args& args) const OVERRIDE {
                if (args.Argc() == 2) {
                    StartMusic( args.Argv(1) , "");
                } else {
                    PrintUsage(args, "/playMusic <file>", "play a music file");
                }
            }

            virtual Cmd::CompletionResult Complete(int argNum, const Cmd::Args&, Str::StringRef prefix) const OVERRIDE {
                if (argNum == 1) {
                    //TODO have a list of supported extensions somewhere and use that?
                    return FS::PakPath::CompleteFilename(prefix, "", "", true, false);
                }

                return {};
            }
    };
    static PlayMusicCmd playMusicRegistration;

    // Stop the current music
    class StopMusicCmd : public Cmd::StaticCmd {
        public:
            StopMusicCmd(): StaticCmd("stopMusic", Cmd::AUDIO, "Stops the currently playing music") {
            }

            virtual void Run(const Cmd::Args&) const OVERRIDE {
                StopMusic();
            }
    };
    static StopMusicCmd stopMusicRegistration;


    // Additional utility functions

    // Some volume slider-tweaking functions, full of heuristics and probably very slow for what they do.
    static float PerceptualToAmplitude(float perceptual) {
        // Conversion from a perceptual audio scale on [0, 1] to an amplitude scale on [0, 1]
        // Assuming the decibel scale is perceptual, naming A the amplitude and B the perceived volume:
        //     B = 3log_10(A + c) + d and A = 10^((B - d)/3) - c
        // With c and d two constants such that the equation holds for A = B = 0 and A = B = 1.
        // Solving gives us:
        //     c = 10^(-d/3) and 10^((1-d)/3) - 10^(-d/3) = 1
        // With Wofram Alpha we get:
        //     d = 3log_10(10^(1/3) - 1) and c = 1 / (10^(1/3) - 1)
        const float PERCEPTUAL_C = 0.866224835960518f;
        const float PERCEPTUAL_D = 0.187108105667604f;

        return std::pow(10.0f, (perceptual - PERCEPTUAL_D) / 3.0f) - PERCEPTUAL_C;
    }

    float SliderToAmplitude(float slider) {
        // We want the users to control the perceptual volume but at the same time have more
        // control over the small value than over bigger ones so we tweak the slider value
        // first.
        float perceptual = slider * slider;
        return PerceptualToAmplitude(perceptual);
    }

}
