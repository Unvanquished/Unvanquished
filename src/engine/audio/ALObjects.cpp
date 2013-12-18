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

    // Common functions

    void ClearError() {
        int error = alGetError();

        if (error != AL_NO_ERROR) {
            Log::Warn("Unhandled OpenAL error: %i"); //TODO string error
        }
    }

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
    }

    Buffer::Buffer(Buffer&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Buffer::~Buffer() {
        if (alHandle != 0) {
            alDeleteBuffers(1, &alHandle);
        }
        alHandle = 0;
    }

    unsigned Buffer::Feed(snd_info_t info, void* data) {
        ALuint format = Format(info.width, info.channels);

        ClearError();
		alBufferData(alHandle, format, data, info.size, info.rate);

        return alGetError();
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
    }

    Effect::Effect(Effect&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Effect::~Effect() {
        if (alHandle != 0) {
            alDeleteEffects(1, &alHandle);
        }
        alHandle = 0;
    }

    void Effect::MakeReverb() {
        alEffecti(alHandle, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
    }

    void Effect::SetReverbDensity(float density){
        alEffectf(alHandle, AL_EAXREVERB_DENSITY, density);
    }

    void Effect::SetReverbDiffusion(float diffusion) {
        alEffectf(alHandle, AL_EAXREVERB_DIFFUSION, diffusion);
    }

    void Effect::SetReverbGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAIN, gain);
    }

    void Effect::SetReverbGainHF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAINHF, gain);
    }

    void Effect::SetReverbGainLF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_GAINLF, gain);
    }

    void Effect::SetReverbDecayTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_TIME, time);
    }

    void Effect::SetReverbDecayHFRatio(float ratio) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_HFRATIO, ratio);
    }

    void Effect::SetReverbDecayLFRatio(float ratio) {
        alEffectf(alHandle, AL_EAXREVERB_DECAY_LFRATIO, ratio);
    }

    void Effect::SetReverbReflectionsGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_REFLECTIONS_GAIN, gain);
    }

    void Effect::SetReverbReflectionsDelay(float delay) {
        alEffectf(alHandle, AL_EAXREVERB_REFLECTIONS_DELAY, delay);
    }

    void Effect::SetReverbLateReverbGain(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_LATE_REVERB_GAIN, gain);
    }

    void Effect::SetReverbLateReverbDelay(float delay) {
        alEffectf(alHandle, AL_EAXREVERB_LATE_REVERB_DELAY, delay);
    }

    void Effect::SetReverbEchoTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_ECHO_TIME, time);
    }

    void Effect::SetReverbEchoDepth(float depth) {
        alEffectf(alHandle, AL_EAXREVERB_ECHO_DEPTH, depth);
    }

    void Effect::SetReverbModulationTime(float time) {
        alEffectf(alHandle, AL_EAXREVERB_MODULATION_TIME, time);
    }

    void Effect::SetReverbModulationDepth(float depth) {
        alEffectf(alHandle, AL_EAXREVERB_MODULATION_DEPTH, depth);
    }

    void Effect::SetReverbAirAbsorptionGainHF(float gain) {
        alEffectf(alHandle, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, gain);
    }

    void Effect::SetReverbHFReference(float reference) {
        alEffectf(alHandle, AL_EAXREVERB_HFREFERENCE, reference);
    }

    void Effect::SetReverbLFReference(float reference) {
        alEffectf(alHandle, AL_EAXREVERB_LFREFERENCE, reference);
    }

    void Effect::SetReverbDelayHFLimit(bool delay) {
        alEffecti(alHandle, AL_EAXREVERB_DECAY_HFLIMIT, delay);
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
    }

    EffectSlot::EffectSlot(EffectSlot&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    EffectSlot::~EffectSlot() {
        if (alHandle != 0) {
            alDeleteAuxiliaryEffectSlots(1, &alHandle);
        }
        alHandle = 0;
    }

    void EffectSlot::SetGain(float gain) {
        alAuxiliaryEffectSlotf(alHandle, AL_EFFECTSLOT_GAIN, gain);
    }

    void EffectSlot::SetEffect(Effect& effect) {
        alAuxiliaryEffectSloti(alHandle, AL_EFFECTSLOT_EFFECT, effect);
    }

    EffectSlot::operator unsigned() const {
        return alHandle;
    }

    // Listener function implementations

    void SetListenerGain(float gain) {
        alListenerf(AL_GAIN, gain);
    }

    void SetListenerPosition(const vec3_t position) {
        alListenerfv(AL_POSITION, position);
    }

    void SetListenerVelocity(const vec3_t velocity) {
        alListenerfv(AL_VELOCITY, velocity);
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
    }

    // OpenAL state functions

    void SetSpeedOfSound(float speed) {
        alSpeedOfSound(speed);
    }

    void SetDopplerExaggerationFactor(float factor) {
        alDopplerFactor(factor);
    }

    // Source implementation

    Source::Source() {
        alGenSources(1, &alHandle);
    }

    Source::Source(Source&& other) {
        alHandle = other.alHandle;
        other.alHandle = 0;
    }

    Source::~Source() {
        if (alHandle != 0) {
            alDeleteSources(1, &alHandle);
        }
        alHandle = 0;
    }

    bool Source::IsStopped() {
        ALint state;
        alGetSourcei(alHandle, AL_SOURCE_STATE, &state);
        return state == AL_STOPPED;
    }

    void Source::Play() {
        alSourcePlay(alHandle);
    }

    void Source::Pause() {
        alSourcePause(alHandle);
    }

    void Source::Stop() {
        alSourceStop(alHandle);
    }

    void Source::SetGain(float gain) {
        alSourcef(alHandle, AL_GAIN, gain);
    }

    void Source::SetPosition(const vec3_t position) {
        alSourcefv(alHandle, AL_POSITION, position);
    }

    void Source::SetVelocity(const vec3_t velocity) {
        alSourcefv(alHandle, AL_VELOCITY, velocity);
    }

    void Source::SetBuffer(Buffer& buffer) {
        alSourcei(alHandle, AL_BUFFER, buffer);
    }

    void Source::SetLooping(bool loop) {
        alSourcei(alHandle, AL_LOOPING, loop);
    }

    void Source::SetRolloff(float factor) {
        alSourcef(alHandle, AL_ROLLOFF_FACTOR, factor);
    }

    void Source::SetReferenceDistance(float distance) {
        alSourcef(alHandle, AL_REFERENCE_DISTANCE, distance);
    }

    void Source::SetRelative(bool relative) {
        alSourcei(alHandle, AL_SOURCE_RELATIVE, relative);
    }

    void Source::SetSlotEffect(int slot, Effect& effect) {
        slots[slot].SetEffect(effect);
    }

    void Source::EnableSlot(int slot) {
        alSource3i(alHandle, AL_AUXILIARY_SEND_FILTER, slots[slot], slot, AL_FILTER_NULL);
    }

    void Source::DisableSlot(int slot) {
        alSource3i(alHandle, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, slot, AL_FILTER_NULL);
    }

    Source::operator unsigned() const {
        return alHandle;
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
