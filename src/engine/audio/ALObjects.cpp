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

#define AL_ALEXT_PROTOTYPES
#include <al.h>
#include <alc.h>
#include <efx.h>
#include <efx-presets.h>

namespace Audio {
namespace AL {

    #define CHECK_ERROR() ClearError(__LINE__)

    std::string ErrorToString(int error) {
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

    //TODO add a cvar or a DEBUG flag to avoid making too many checks?
    int  ClearError(int line = -1) {
        int error = alGetError();

        if (error != AL_NO_ERROR) {
            if (line < 0) {
                Log::Warn("Unhandled OpenAL error: %s", ErrorToString(error));
            } else {
                Log::Warn("Unhandled OpenAL error on line %i: %s", line, ErrorToString(error));
            }
        }

        return error;
    }

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
        CHECK_ERROR();
    }

    Buffer::Buffer(Buffer&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Buffer::~Buffer() {
        if (alHandle != 0) {
            alDeleteBuffers(1, &alHandle);
            CHECK_ERROR();
        }
        alHandle = 0;
    }

    unsigned Buffer::Feed(snd_info_t info, void* data) {
        ALuint format = Format(info.width, info.channels);

        CHECK_ERROR();
		alBufferData(alHandle, format, data, info.size, info.rate);

        return CHECK_ERROR();
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

    ReverbEffectPreset& GetHangarEffectPreset() {
        static ReverbEffectPreset preset(EFX_REVERB_PRESET_HANGAR);
        return preset;
    }

    // Effect implementation

    Effect::Effect() {
        alGenEffects(1, &alHandle);
        CHECK_ERROR();
    }

    Effect::Effect(Effect&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Effect::~Effect() {
        if (alHandle != 0) {
            alDeleteEffects(1, &alHandle);
            CHECK_ERROR();
        }
        alHandle = 0;
    }

    void Effect::MakeReverb() {
        alEffecti(alHandle, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
        CHECK_ERROR();
    }

    void Effect::SetReverbDensity(float density){
        alEffectf(alHandle, AL_EAXREVERB_DENSITY, density);
        CHECK_ERROR();
    }

    void Effect::SetReverbDiffusion(float diffusion) {
        alEffectf(alHandle, AL_EAXREVERB_DIFFUSION, diffusion);
        CHECK_ERROR();
    }

    void Effect::SetReverbGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAIN, gain);
        CHECK_ERROR();
    }

    void Effect::SetReverbGainHF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAINHF, gain);
        CHECK_ERROR();
    }

    void Effect::SetReverbGainLF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAINLF, gain);
        CHECK_ERROR();
    }

    void Effect::SetReverbDecayTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_TIME, time);
        CHECK_ERROR();
    }

    void Effect::SetReverbDecayHFRatio(float ratio) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_HFRATIO, ratio);
        CHECK_ERROR();
    }

    void Effect::SetReverbDecayLFRatio(float ratio) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_LFRATIO, ratio);
        CHECK_ERROR();
    }

    void Effect::SetReverbReflectionsGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_REFLECTIONS_GAIN, gain);
        CHECK_ERROR();
    }

    void Effect::SetReverbReflectionsDelay(float delay) {
        alEffectf(alHandle, AL_EAXREVERB_REFLECTIONS_DELAY, delay);
        CHECK_ERROR();
    }

    void Effect::SetReverbLateReverbGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_LATE_REVERB_GAIN, gain);
        CHECK_ERROR();
    }

    void Effect::SetReverbLateReverbDelay(float delay) {
        alEffectf(alHandle, AL_EAXREVERB_LATE_REVERB_DELAY, delay);
        CHECK_ERROR();
    }

    void Effect::SetReverbEchoTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_ECHO_TIME, time);
        CHECK_ERROR();
    }

    void Effect::SetReverbEchoDepth(float depth) {
        alEffectf(alHandle, AL_EAXREVERB_ECHO_DEPTH, depth);
        CHECK_ERROR();
    }

    void Effect::SetReverbModulationTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_MODULATION_TIME, time);
        CHECK_ERROR();
    }

    void Effect::SetReverbModulationDepth(float depth) {
        alEffectf(alHandle, AL_EAXREVERB_MODULATION_DEPTH, depth);
        CHECK_ERROR();
    }

    void Effect::SetReverbAirAbsorptionGainHF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, gain);
        CHECK_ERROR();
    }

    void Effect::SetReverbHFReference(float reference) {
        alEffectf(alHandle, AL_EAXREVERB_HFREFERENCE, reference);
        CHECK_ERROR();
    }

    void Effect::SetReverbLFReference(float reference) {
        alEffectf(alHandle, AL_EAXREVERB_LFREFERENCE, reference);
        CHECK_ERROR();
    }

    void Effect::SetReverbDelayHFLimit(bool delay) {
        alEffecti(alHandle, AL_EAXREVERB_DECAY_HFLIMIT, delay);
        CHECK_ERROR();
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
        CHECK_ERROR();
    }

    EffectSlot::EffectSlot(EffectSlot&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    EffectSlot::~EffectSlot() {
        if (alHandle != 0) {
            alDeleteAuxiliaryEffectSlots(1, &alHandle);
            CHECK_ERROR();
        }
        alHandle = 0;
    }

    void EffectSlot::SetGain(float gain) {
        alAuxiliaryEffectSlotf(alHandle, AL_EFFECTSLOT_GAIN, gain);
        CHECK_ERROR();
    }

    void EffectSlot::SetEffect(Effect& effect) {
        alAuxiliaryEffectSloti(alHandle, AL_EFFECTSLOT_EFFECT, effect);
        CHECK_ERROR();
    }

    EffectSlot::operator unsigned() const {
        return alHandle;
    }

    // Listener function implementations

    void SetListenerGain(float gain) {
        alListenerf(AL_GAIN, gain);
        CHECK_ERROR();
    }

    void SetListenerPosition(const vec3_t position) {
        alListenerfv(AL_POSITION, position);
        CHECK_ERROR();
    }

    void SetListenerVelocity(const vec3_t velocity) {
        alListenerfv(AL_VELOCITY, velocity);
        CHECK_ERROR();
    }

    void SetListenerOrientation(const vec3_t orientation[3]) {
        float alOrientation[6] = {
            orientation[0][0],
            orientation[0][1],
            orientation[0][2],
            orientation[2][0],
            orientation[2][1],
            orientation[2][2]
        };

        alListenerfv(AL_ORIENTATION, alOrientation);
        CHECK_ERROR();
    }

    // OpenAL state functions

    void SetSpeedOfSound(float speed) {
        alSpeedOfSound(speed);
        CHECK_ERROR();
    }

    void SetDopplerExaggerationFactor(float factor) {
        alDopplerFactor(factor);
        CHECK_ERROR();
    }

    // Source implementation

    Source::Source() {
        alGenSources(1, &alHandle);
        CHECK_ERROR();
    }

    Source::Source(Source&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Source::~Source() {
        RemoveAllQueuedBuffers();
        if (alHandle != 0) {
            alDeleteSources(1, &alHandle);
            CHECK_ERROR();
        }
        alHandle = 0;
    }

    void Source::Play() {
        alSourcePlay(alHandle);
        CHECK_ERROR();
    }

    void Source::Pause() {
        alSourcePause(alHandle);
        CHECK_ERROR();
    }

    void Source::Stop() {
        alSourceStop(alHandle);
        CHECK_ERROR();
    }

    bool Source::IsStopped() {
        ALint state;
        alGetSourcei(alHandle, AL_SOURCE_STATE, &state);
        CHECK_ERROR();
        return state == AL_STOPPED;
    }

    void Source::SetBuffer(Buffer& buffer) {
        alSourcei(alHandle, AL_BUFFER, buffer);
        CHECK_ERROR();
    }

    void Source::QueueBuffer(Buffer buffer) {
        ALuint bufHandle = buffer.Acquire();
        alSourceQueueBuffers(alHandle, 1, &bufHandle);
        CHECK_ERROR();
    }

    int Source::GetNumProcessedBuffers() {
        int res;
        alGetSourcei(alHandle, AL_BUFFERS_PROCESSED, &res);
        CHECK_ERROR();
        return res;
    }

    int Source::GetNumQueuedBuffers() {
        int res;
        alGetSourcei(alHandle, AL_BUFFERS_QUEUED, &res);
        CHECK_ERROR();
        return res;
    }

    Buffer Source::PopBuffer() {
        unsigned bufferHandle;
        alSourceUnqueueBuffers(alHandle, 1, &bufferHandle);
        CHECK_ERROR();
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
        CHECK_ERROR();
    }

    void Source::SetGain(float gain) {
        alSourcef(alHandle, AL_GAIN, gain);
        CHECK_ERROR();
    }

    void Source::SetPosition(const vec3_t position) {
        alSourcefv(alHandle, AL_POSITION, position);
        CHECK_ERROR();
    }

    void Source::SetVelocity(const vec3_t velocity) {
        alSourcefv(alHandle, AL_VELOCITY, velocity);
        CHECK_ERROR();
    }

    void Source::SetLooping(bool loop) {
        alSourcei(alHandle, AL_LOOPING, loop);
        CHECK_ERROR();
    }

    void Source::SetRolloff(float factor) {
        alSourcef(alHandle, AL_ROLLOFF_FACTOR, factor);
        CHECK_ERROR();
    }

    void Source::SetReferenceDistance(float distance) {
        alSourcef(alHandle, AL_REFERENCE_DISTANCE, distance);
        CHECK_ERROR();
    }

    void Source::SetRelative(bool relative) {
        alSourcei(alHandle, AL_SOURCE_RELATIVE, relative);
        CHECK_ERROR();
    }

    void Source::EnableEffect(int slot, EffectSlot& effect) {
        alSource3i(alHandle, AL_AUXILIARY_SEND_FILTER, effect, slot, AL_FILTER_NULL);
        CHECK_ERROR();
    }

    void Source::DisableEffect(int slot) {
        alSource3i(alHandle, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, slot, AL_FILTER_NULL);
        CHECK_ERROR();
    }

    Source::operator unsigned() const {
        return alHandle;
    }

    int Source::GetType() {
        ALint type;
        alGetSourcei(alHandle, AL_SOURCE_TYPE, &type);
        CHECK_ERROR();
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

    std::vector<std::string> Device::ListByName() {
        const char* list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);

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
}
}
