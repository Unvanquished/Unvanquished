/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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

#define AL_ALEXT_PROTOTYPES
#include <al.h>
#include <alc.h>
#include <efx.h>
#include <efx-presets.h>

namespace Audio {
namespace AL {

    std::string ALErrorToString(int error) {
        switch (error) {
            case AL_NO_ERROR:
                return "No Error";

            case AL_INVALID_NAME:
                return "Invalid name";

            case AL_INVALID_ENUM:
                return "Invalid enum";

            case AL_INVALID_VALUE:
                return "Invalid value";

            case AL_INVALID_OPERATION:
                return "Invalid operation";

            case AL_OUT_OF_MEMORY:
                return "Out of memory";

            default:
                return "Unknown OpenAL error";
        }
    }

    // Used to avoid unnecessary calls to alGetError
    static Cvar::Cvar<bool> checkAllCalls("audio.al.checkAllCalls", "check all OpenAL calls for errors", Cvar::NONE, false);
    int ClearALError(int line = -1) {
        int error = alGetError();

        if (error != AL_NO_ERROR) {
            if (line >= 0) {
                audioLogs.Warn("Unhandled OpenAL error on line %i: %s", line, ALErrorToString(error));
            } else if (checkAllCalls.Get()) {
                audioLogs.Warn("Unhandled OpenAL error: %s", ALErrorToString(error));
            }
        }

        return error;
    }

    void CheckALError(int line) {
        if (checkAllCalls.Get()) {
            ClearALError(line);
        }
    }

    #define CHECK_AL_ERROR() CheckALError(__LINE__)

    // Converts an snd_codec_t format to an OpenAL format
    ALuint Format(int width, int channels) {
        if (width == 1) {

            if (channels == 1) {
                return AL_FORMAT_MONO8;
            } else if (channels == 2) {
                return AL_FORMAT_STEREO8;
            }

        } else if (width == 2) {

            if (channels == 1) {
                return AL_FORMAT_MONO16;
            } else if (channels == 2) {
                return AL_FORMAT_STEREO16;
            }
        }

        return AL_FORMAT_MONO16;
    }

    // Buffer implementation

    Buffer::Buffer() {
        alGenBuffers(1, &alHandle);
        CHECK_AL_ERROR();
    }

    Buffer::Buffer(Buffer&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Buffer::~Buffer() {
        if (alHandle != 0) {
            alDeleteBuffers(1, &alHandle);
            CHECK_AL_ERROR();
        }
        alHandle = 0;
    }

    unsigned Buffer::Feed(const AudioData& audioData)
    {
	    ALuint format = Format(audioData.byteDepth, audioData.numberOfChannels);

	    CHECK_AL_ERROR();
	    alBufferData(alHandle, format, audioData.rawSamples.get(), audioData.size,
	                 audioData.sampleRate);

	    return ClearALError();
    }

    Buffer::Buffer(unsigned handle): alHandle(handle) {
    }

    unsigned Buffer::Acquire() {
        unsigned temp = alHandle;
        alHandle = 0;
        return temp;
    }

    Buffer::operator unsigned() const {
        return alHandle;
    }

    // ReverbEffectPreset implementation

    struct ReverbEffectPreset {
        ReverbEffectPreset(EFXEAXREVERBPROPERTIES builtinPreset);

        float density;
        float diffusion;
        float gain;
        float gainHF;
        float gainLF;
        float decayTime;
        float decayHFRatio;
        float decayLFRatio;
        float reflectionsGain;
        float reflectionsDelay;
        float lateReverbGain;
        float lateReverbDelay;
        float echoTime;
        float echoDepth;
        float modulationTime;
        float modulationDepth;
        float airAbsorptionGainHF;
        float HFReference;
        float LFReference;
        bool decayHFLimit;
    };

    ReverbEffectPreset::ReverbEffectPreset(EFXEAXREVERBPROPERTIES builtinPreset):
        density(builtinPreset.flDensity),
        diffusion(builtinPreset.flDiffusion),
        gain(builtinPreset.flGain),
        gainHF(builtinPreset.flGainHF),
        gainLF(builtinPreset.flGainLF),
        decayTime(builtinPreset.flDecayTime),
        decayHFRatio(builtinPreset.flDecayHFRatio),
        decayLFRatio(builtinPreset.flDecayLFRatio),
        reflectionsGain(builtinPreset.flReflectionsGain),
        reflectionsDelay(builtinPreset.flReflectionsDelay),
        lateReverbGain(builtinPreset.flLateReverbGain),
        lateReverbDelay(builtinPreset.flLateReverbDelay),
        echoTime(builtinPreset.flEchoTime),
        echoDepth(builtinPreset.flEchoDepth),
        modulationTime(builtinPreset.flModulationTime),
        modulationDepth(builtinPreset.flModulationDepth),
        airAbsorptionGainHF(builtinPreset.flAirAbsorptionGainHF),
        HFReference(builtinPreset.flHFReference),
        LFReference(builtinPreset.flLFReference),
        decayHFLimit(builtinPreset.iDecayHFLimit) {
    }

    std::unordered_map<std::string, ReverbEffectPreset> presets;
    static bool presetsInitialized = false;
    void InitEffectPresets() {
        if (presetsInitialized) {
            return;
        }
        presetsInitialized = true;

        #define ADD_PRESET(name) presets.insert(std::make_pair(Str::ToLower(#name), ReverbEffectPreset(EFX_REVERB_PRESET_##name)));
        ADD_PRESET(GENERIC)
        ADD_PRESET(PADDEDCELL)
        ADD_PRESET(ROOM)
        ADD_PRESET(BATHROOM)
        ADD_PRESET(LIVINGROOM)
        ADD_PRESET(STONEROOM)
        ADD_PRESET(AUDITORIUM)
        ADD_PRESET(CONCERTHALL)
        ADD_PRESET(CAVE)
        ADD_PRESET(ARENA)
        ADD_PRESET(HANGAR)
        ADD_PRESET(CARPETEDHALLWAY)
        ADD_PRESET(HALLWAY)
        ADD_PRESET(STONECORRIDOR)
        ADD_PRESET(ALLEY)
        ADD_PRESET(FOREST)
        ADD_PRESET(CITY)
        ADD_PRESET(MOUNTAINS)
        ADD_PRESET(QUARRY)
        ADD_PRESET(PLAIN)
        ADD_PRESET(PARKINGLOT)
        ADD_PRESET(SEWERPIPE)
        ADD_PRESET(UNDERWATER)
        ADD_PRESET(DRUGGED)
        ADD_PRESET(DIZZY)
        ADD_PRESET(PSYCHOTIC)
        ADD_PRESET(CASTLE_SMALLROOM)
        ADD_PRESET(CASTLE_SHORTPASSAGE)
        ADD_PRESET(CASTLE_MEDIUMROOM)
        ADD_PRESET(CASTLE_LARGEROOM)
        ADD_PRESET(CASTLE_LONGPASSAGE)
        ADD_PRESET(CASTLE_HALL)
        ADD_PRESET(CASTLE_CUPBOARD)
        ADD_PRESET(CASTLE_COURTYARD)
        ADD_PRESET(CASTLE_ALCOVE)
        ADD_PRESET(FACTORY_SMALLROOM)
        ADD_PRESET(FACTORY_SHORTPASSAGE)
        ADD_PRESET(FACTORY_MEDIUMROOM)
        ADD_PRESET(FACTORY_LARGEROOM)
        ADD_PRESET(FACTORY_LONGPASSAGE)
        ADD_PRESET(FACTORY_HALL)
        ADD_PRESET(FACTORY_CUPBOARD)
        ADD_PRESET(FACTORY_COURTYARD)
        ADD_PRESET(FACTORY_ALCOVE)
        ADD_PRESET(ICEPALACE_SMALLROOM)
        ADD_PRESET(ICEPALACE_SHORTPASSAGE)
        ADD_PRESET(ICEPALACE_MEDIUMROOM)
        ADD_PRESET(ICEPALACE_LARGEROOM)
        ADD_PRESET(ICEPALACE_LONGPASSAGE)
        ADD_PRESET(ICEPALACE_HALL)
        ADD_PRESET(ICEPALACE_CUPBOARD)
        ADD_PRESET(ICEPALACE_COURTYARD)
        ADD_PRESET(ICEPALACE_ALCOVE)
        ADD_PRESET(SPACESTATION_SMALLROOM)
        ADD_PRESET(SPACESTATION_SHORTPASSAGE)
        ADD_PRESET(SPACESTATION_MEDIUMROOM)
        ADD_PRESET(SPACESTATION_LARGEROOM)
        ADD_PRESET(SPACESTATION_LONGPASSAGE)
        ADD_PRESET(SPACESTATION_HALL)
        ADD_PRESET(SPACESTATION_CUPBOARD)
        ADD_PRESET(SPACESTATION_ALCOVE)
        ADD_PRESET(WOODEN_SMALLROOM)
        ADD_PRESET(WOODEN_SHORTPASSAGE)
        ADD_PRESET(WOODEN_MEDIUMROOM)
        ADD_PRESET(WOODEN_LARGEROOM)
        ADD_PRESET(WOODEN_LONGPASSAGE)
        ADD_PRESET(WOODEN_HALL)
        ADD_PRESET(WOODEN_CUPBOARD)
        ADD_PRESET(WOODEN_COURTYARD)
        ADD_PRESET(WOODEN_ALCOVE)
        ADD_PRESET(SPORT_EMPTYSTADIUM)
        ADD_PRESET(SPORT_SQUASHCOURT)
        ADD_PRESET(SPORT_SMALLSWIMMINGPOOL)
        ADD_PRESET(SPORT_LARGESWIMMINGPOOL)
        ADD_PRESET(SPORT_GYMNASIUM)
        ADD_PRESET(SPORT_FULLSTADIUM)
        ADD_PRESET(SPORT_STADIUMTANNOY)
        ADD_PRESET(PREFAB_WORKSHOP)
        ADD_PRESET(PREFAB_SCHOOLROOM)
        ADD_PRESET(PREFAB_PRACTISEROOM)
        ADD_PRESET(PREFAB_OUTHOUSE)
        ADD_PRESET(PREFAB_CARAVAN)
        ADD_PRESET(DOME_TOMB)
        ADD_PRESET(PIPE_SMALL)
        ADD_PRESET(DOME_SAINTPAULS)
        ADD_PRESET(PIPE_LONGTHIN)
        ADD_PRESET(PIPE_LARGE)
        ADD_PRESET(PIPE_RESONANT)
        ADD_PRESET(OUTDOORS_BACKYARD)
        ADD_PRESET(OUTDOORS_ROLLINGPLAINS)
        ADD_PRESET(OUTDOORS_DEEPCANYON)
        ADD_PRESET(OUTDOORS_CREEK)
        ADD_PRESET(OUTDOORS_VALLEY)
        ADD_PRESET(MOOD_HEAVEN)
        ADD_PRESET(MOOD_HELL)
        ADD_PRESET(MOOD_MEMORY)
        ADD_PRESET(DRIVING_COMMENTATOR)
        ADD_PRESET(DRIVING_PITGARAGE)
        ADD_PRESET(DRIVING_INCAR_RACER)
        ADD_PRESET(DRIVING_INCAR_SPORTS)
        ADD_PRESET(DRIVING_INCAR_LUXURY)
        ADD_PRESET(DRIVING_FULLGRANDSTAND)
        ADD_PRESET(DRIVING_EMPTYGRANDSTAND)
        ADD_PRESET(DRIVING_TUNNEL)
        ADD_PRESET(CITY_STREETS)
        ADD_PRESET(CITY_SUBWAY)
        ADD_PRESET(CITY_MUSEUM)
        ADD_PRESET(CITY_LIBRARY)
        ADD_PRESET(CITY_UNDERPASS)
        ADD_PRESET(CITY_ABANDONED)
        ADD_PRESET(DUSTYROOM)
        ADD_PRESET(CHAPEL)
        ADD_PRESET(SMALLWATERROOM)
    }

    ReverbEffectPreset* GetPresetByName(Str::StringRef name) {
        auto it = presets.find(name);
        if (it == presets.end()) {
            return nullptr;
        } else {
            //We can do this because the map never changes
            return &it->second;
        }
    }

    std::vector<std::string> ListPresetNames() {
        std::vector<std::string> res;

        for (const auto& it: presets) {
            res.push_back(it.first);
        }

        return res;
    }

    // Effect implementation

    Effect::Effect() {
        alGenEffects(1, &alHandle);
        CHECK_AL_ERROR();
    }

    Effect::Effect(Effect&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Effect::~Effect() {
        if (alHandle != 0) {
            alDeleteEffects(1, &alHandle);
            CHECK_AL_ERROR();
        }
        alHandle = 0;
    }

    void Effect::MakeReverb() {
        alEffecti(alHandle, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbDensity(float density){
        alEffectf(alHandle, AL_EAXREVERB_DENSITY, density);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbDiffusion(float diffusion) {
        alEffectf(alHandle, AL_EAXREVERB_DIFFUSION, diffusion);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAIN, gain);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbGainHF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAINHF, gain);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbGainLF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAINLF, gain);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbDecayTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_TIME, time);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbDecayHFRatio(float ratio) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_HFRATIO, ratio);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbDecayLFRatio(float ratio) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_LFRATIO, ratio);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbReflectionsGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_REFLECTIONS_GAIN, gain);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbReflectionsDelay(float delay) {
        alEffectf(alHandle, AL_EAXREVERB_REFLECTIONS_DELAY, delay);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbLateReverbGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_LATE_REVERB_GAIN, gain);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbLateReverbDelay(float delay) {
        alEffectf(alHandle, AL_EAXREVERB_LATE_REVERB_DELAY, delay);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbEchoTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_ECHO_TIME, time);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbEchoDepth(float depth) {
        alEffectf(alHandle, AL_EAXREVERB_ECHO_DEPTH, depth);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbModulationTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_MODULATION_TIME, time);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbModulationDepth(float depth) {
        alEffectf(alHandle, AL_EAXREVERB_MODULATION_DEPTH, depth);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbAirAbsorptionGainHF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, gain);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbHFReference(float reference) {
        alEffectf(alHandle, AL_EAXREVERB_HFREFERENCE, reference);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbLFReference(float reference) {
        alEffectf(alHandle, AL_EAXREVERB_LFREFERENCE, reference);
        CHECK_AL_ERROR();
    }

    void Effect::SetReverbDelayHFLimit(bool delay) {
        alEffecti(alHandle, AL_EAXREVERB_DECAY_HFLIMIT, delay);
        CHECK_AL_ERROR();
    }

    void Effect::ApplyReverbPreset(ReverbEffectPreset& preset) {
        MakeReverb();
        SetReverbDensity(preset.density);
        SetReverbDiffusion(preset.diffusion);
        SetReverbGain(preset.gain);
        SetReverbGainHF(preset.gainHF);
        SetReverbGainLF(preset.gainLF);
        SetReverbDecayTime(preset.decayTime);
        SetReverbDecayHFRatio(preset.decayHFRatio);
        SetReverbDecayLFRatio(preset.decayLFRatio);
        SetReverbReflectionsGain(preset.reflectionsGain);
        SetReverbReflectionsDelay(preset.reflectionsDelay);
        SetReverbLateReverbGain(preset.lateReverbGain);
        SetReverbLateReverbDelay(preset.lateReverbDelay);
        SetReverbEchoTime(preset.echoTime);
        SetReverbEchoDepth(preset.echoDepth);
        SetReverbModulationTime(preset.modulationTime);
        SetReverbModulationDepth(preset.modulationDepth);
        SetReverbAirAbsorptionGainHF(preset.airAbsorptionGainHF);
        SetReverbHFReference(preset.HFReference);
        SetReverbLFReference(preset.LFReference);
        SetReverbDelayHFLimit(preset.decayHFLimit);
    }

    Effect::operator unsigned() const {
        return alHandle;
    }

    // EffectSlot Implementation

    EffectSlot::EffectSlot() {
        alGenAuxiliaryEffectSlots(1, &alHandle);
        CHECK_AL_ERROR();
    }

    EffectSlot::EffectSlot(EffectSlot&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    EffectSlot::~EffectSlot() {
        if (alHandle != 0) {
            alDeleteAuxiliaryEffectSlots(1, &alHandle);
            CHECK_AL_ERROR();
        }
        alHandle = 0;
    }

    void EffectSlot::SetGain(float gain) {
        alAuxiliaryEffectSlotf(alHandle, AL_EFFECTSLOT_GAIN, gain);
        CHECK_AL_ERROR();
    }

    void EffectSlot::SetEffect(Effect& effect) {
        alAuxiliaryEffectSloti(alHandle, AL_EFFECTSLOT_EFFECT, effect);
        CHECK_AL_ERROR();
    }

    EffectSlot::operator unsigned() const {
        return alHandle;
    }

    // Listener function implementations

    void SetListenerGain(float gain) {
        alListenerf(AL_GAIN, gain);
        CHECK_AL_ERROR();
    }

    void SetListenerPosition(Vec3 position) {
        alListenerfv(AL_POSITION, position.Data());
        CHECK_AL_ERROR();
    }

    void SetListenerVelocity(Vec3 velocity) {
        alListenerfv(AL_VELOCITY, velocity.Data());
        CHECK_AL_ERROR();
    }

    void SetListenerOrientation(const Vec3 orientation[3]) {
        float alOrientation[6] = {
            orientation[0][0],
            orientation[0][1],
            orientation[0][2],
            orientation[2][0],
            orientation[2][1],
            orientation[2][2]
        };

        alListenerfv(AL_ORIENTATION, alOrientation);
        CHECK_AL_ERROR();
    }

    // OpenAL state functions

    void SetInverseDistanceModel() {
        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
        CHECK_AL_ERROR();
    }

    void SetSpeedOfSound(float speed) {
        alSpeedOfSound(speed);
        CHECK_AL_ERROR();
    }

    void SetDopplerExaggerationFactor(float factor) {
        alDopplerFactor(factor);
        CHECK_AL_ERROR();
    }

    // Source implementation

    Source::Source() {
        alGenSources(1, &alHandle);
        CHECK_AL_ERROR();
    }

    Source::Source(Source&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Source::~Source() {
        RemoveAllQueuedBuffers();
        if (alHandle != 0) {
            alDeleteSources(1, &alHandle);
            CHECK_AL_ERROR();
        }
        alHandle = 0;
    }

    void Source::Play() {
        alSourcePlay(alHandle);
        CHECK_AL_ERROR();
    }

    void Source::Pause() {
        alSourcePause(alHandle);
        CHECK_AL_ERROR();
    }

    void Source::Stop() {
        alSourceStop(alHandle);
        CHECK_AL_ERROR();
    }

    bool Source::IsStopped() {
        ALint state;
        alGetSourcei(alHandle, AL_SOURCE_STATE, &state);
        CHECK_AL_ERROR();
        return state == AL_STOPPED;
    }

    void Source::SetBuffer(Buffer& buffer) {
        alSourcei(alHandle, AL_BUFFER, buffer);
        CHECK_AL_ERROR();
    }

    void Source::QueueBuffer(Buffer buffer) {
        ALuint bufHandle = buffer.Acquire();
        alSourceQueueBuffers(alHandle, 1, &bufHandle);
        CHECK_AL_ERROR();
    }

    int Source::GetNumProcessedBuffers() {
        int res;
        alGetSourcei(alHandle, AL_BUFFERS_PROCESSED, &res);
        CHECK_AL_ERROR();
        return res;
    }

    int Source::GetNumQueuedBuffers() {
        int res;
        alGetSourcei(alHandle, AL_BUFFERS_QUEUED, &res);
        CHECK_AL_ERROR();
        return res;
    }

    Buffer Source::PopBuffer() {
        unsigned bufferHandle;
        alSourceUnqueueBuffers(alHandle, 1, &bufferHandle);
        CHECK_AL_ERROR();
        return {bufferHandle};
    }

    void Source::RemoveAllQueuedBuffers() {
        // OpenAL gives an error if the source isn't stopped or if it is an AL_STATIC source.
        if (GetType() != AL_STREAMING) {
            return;
        }
        Stop();
        int toBeRemoved = GetNumQueuedBuffers();
        while (toBeRemoved --> 0) {
            // The buffer will be copied and destroyed immediately as well as it's OpenAL buffer
            PopBuffer();
        }
    }

    void Source::ResetBuffer() {
        alSourcei(alHandle, AL_BUFFER, 0);
        CHECK_AL_ERROR();
    }

    void Source::SetGain(float gain) {
        alSourcef(alHandle, AL_GAIN, gain);
        CHECK_AL_ERROR();
    }

    void Source::SetPosition(Vec3 position) {
        alSourcefv(alHandle, AL_POSITION, position.Data());
        CHECK_AL_ERROR();
    }

    void Source::SetVelocity(Vec3 velocity) {
        alSourcefv(alHandle, AL_VELOCITY, velocity.Data());
        CHECK_AL_ERROR();
    }

    void Source::SetLooping(bool loop) {
        alSourcei(alHandle, AL_LOOPING, loop);
        CHECK_AL_ERROR();
    }

    void Source::SetRolloff(float factor) {
        alSourcef(alHandle, AL_ROLLOFF_FACTOR, factor);
        CHECK_AL_ERROR();
    }

    void Source::SetReferenceDistance(float distance) {
        alSourcef(alHandle, AL_REFERENCE_DISTANCE, distance);
        CHECK_AL_ERROR();
    }

    void Source::SetRelative(bool relative) {
        alSourcei(alHandle, AL_SOURCE_RELATIVE, relative);
        CHECK_AL_ERROR();
    }

    void Source::EnableEffect(int slot, EffectSlot& effect) {
        alSource3i(alHandle, AL_AUXILIARY_SEND_FILTER, effect, slot, AL_FILTER_NULL);
        CHECK_AL_ERROR();
    }

    void Source::DisableEffect(int slot) {
        alSource3i(alHandle, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, slot, AL_FILTER_NULL);
        CHECK_AL_ERROR();
    }

    Source::operator unsigned() const {
        return alHandle;
    }

    int Source::GetType() {
        ALint type;
        alGetSourcei(alHandle, AL_SOURCE_TYPE, &type);
        CHECK_AL_ERROR();
        return type;
    }

    // Implementation of Device

    Device* Device::FromName(Str::StringRef name) {
        ALCdevice* alHandle = alcOpenDevice(name.c_str());
        if (alHandle) {
            return new Device(alHandle);
        } else {
            return nullptr;
        }
    }

    Device* Device::GetDefaultDevice() {
        ALCdevice* alHandle = alcOpenDevice(nullptr);
        if (alHandle) {
            return new Device(alHandle);
        } else {
            return nullptr;
        }
    }

    Device::Device(Device&& other) {
        alHandle = other.alHandle;
        other.alHandle = nullptr;
    }

    Device::~Device() {
        if (alHandle) {
            alcCloseDevice((ALCdevice*)alHandle);
        }
        alHandle = nullptr;
    }

    Device::operator void*() {
        return alHandle;
    }

    std::string Device::DefaultDeviceName() {
        return alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    }

    std::vector<std::string> Device::ListByName() {
        const char* list = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

        if (!list) {
            return {};
        }

        // OpenAL gives the concatenation of null-terminated strings, followed by a '\0' (it ends with a double '\0')
        std::vector<std::string> res;
        while (*list) {
            res.push_back(list);
            list += res.back().size() + 1;
        }

        return res;
    }

    Device::Device(void* alHandle): alHandle(alHandle) {
    }

    // Implementation of Context

    Context* Context::GetDefaultContext(Device& device) {
        ALCcontext* alHandle = alcCreateContext((ALCdevice*)(void*)device, nullptr);
        if (alHandle) {
            return new Context(alHandle);
        } else {
            return nullptr;
        }
    }

    Context::Context(Context&& other) {
        alHandle = other.alHandle;
        other.alHandle = nullptr;
    }

    Context::~Context() {
        if (alHandle) {
            alcDestroyContext((ALCcontext*)alHandle);
        }
        alHandle = nullptr;
    }

    void Context::MakeCurrent() {
        alcMakeContextCurrent((ALCcontext*)alHandle);
    }

    Context::operator void*() {
        return alHandle;
    }

    Context::Context(void* alHandle): alHandle(alHandle) {
    }

    // Implementation of CaptureDevice

    CaptureDevice* CaptureDevice::FromName(Str::StringRef name, int rate) {
        ALCdevice* alHandle = alcCaptureOpenDevice(name.c_str(), rate, AL_FORMAT_MONO16, 2 * rate / 16);
        if (alHandle) {
            return new CaptureDevice(alHandle);
        } else {
            return nullptr;
        }
    }

    CaptureDevice* CaptureDevice::GetDefaultDevice(int rate) {
        ALCdevice* alHandle = alcCaptureOpenDevice(DefaultDeviceName().c_str(), rate, AL_FORMAT_MONO16, 2 * rate / 16);
        if (alHandle) {
            return new CaptureDevice(alHandle);
        } else {
            return nullptr;
        }
    }

    CaptureDevice::CaptureDevice(CaptureDevice&& other) {
        alHandle = other.alHandle;
        other.alHandle = nullptr;
    }

    CaptureDevice::~CaptureDevice() {
        if (alHandle) {
            alcCaptureCloseDevice((ALCdevice*)alHandle);
        }
        alHandle = nullptr;
    }

    void CaptureDevice::Start() {
        alcCaptureStart((ALCdevice*)alHandle);
    }

    void CaptureDevice::Stop() {
        alcCaptureStop((ALCdevice*)alHandle);
    }

    int CaptureDevice::GetNumCapturedSamples() {
        int numSamples = 0;
        alcGetIntegerv((ALCdevice*)alHandle, ALC_CAPTURE_SAMPLES, sizeof(numSamples), &numSamples);
        return numSamples;
    }

    void CaptureDevice::Capture(int numSamples, void* buffer) {
        alcCaptureSamples((ALCdevice*)alHandle, buffer, numSamples);
    }

    CaptureDevice::operator void*() {
        return alHandle;
    }

    std::string CaptureDevice::DefaultDeviceName() {
        return alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    }

    std::vector<std::string> CaptureDevice::ListByName() {
        const char* list = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

        if (!list) {
            return {};
        }

        // OpenAL gives the concatenation of null-terminated strings, followed by a '\0' (it ends with a double '\0')
        std::vector<std::string> res;
        while (*list) {
            res.push_back(list);
            list += res.back().size() + 1;
        }

        return res;
    }

    CaptureDevice::CaptureDevice(void* alHandle): alHandle(alHandle) {
    }

    std::string GetSystemInfo(Device* device, CaptureDevice* capture) {
        std::ostringstream o;
        {
            o << "OpenAL System Info:" << std::endl;
            o << " - Vendor: " << alGetString(AL_VENDOR) << std::endl;
            o << " - Version: " << alGetString(AL_VERSION) << std::endl;
            o << " - Renderer: " << alGetString(AL_RENDERER) << std::endl;
            o << " - AL Extensions: " << alGetString(AL_EXTENSIONS) << std::endl;
        }
        if (device) {
            o << " - ALC Extensions: " << alcGetString((ALCdevice*)(void*)*device, ALC_EXTENSIONS) << std::endl;
            o << " - Using Device: " << alcGetString((ALCdevice*)(void*)*device, ALC_ALL_DEVICES_SPECIFIER) << std::endl;
        }
        if (capture) {
            o << " - Using Capture Device: " << alcGetString((ALCdevice*)(void*)*capture, ALC_ALL_DEVICES_SPECIFIER) << std::endl;
        }
        {
            o << " - Available Devices: " << std::endl;
        }
        for (std::string& name : Device::ListByName()) {
            o << "   - " << name << std::endl;
        }
        {
            o << " - Default Device: " << Device::DefaultDeviceName() << std::endl;
            o << " - Available Capture Devices: " << std::endl;
        }
        for (std::string& name : CaptureDevice::ListByName()) {
            o << "   - " << name << std::endl;
        }
        {
            o << " - Default Capture Device: " << CaptureDevice::DefaultDeviceName() << std::endl;
        }

        return o.str();
    }
}
}
