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

#ifndef AUDIO_AL_OBJECTS_H_
#define AUDIO_AL_OBJECTS_H_

#include "../../common/String.h"
#include "snd_codec.h"

namespace Audio {
namespace AL {

    //TODO enum classes for the different ALuint types?

    void ClearError();

    class Buffer {
        public:
            Buffer();
            Buffer(Buffer&& other);
            ~Buffer();

            unsigned Feed(snd_info_t info, void* data);

            Buffer(unsigned handle);
            unsigned Acquire();

            operator unsigned() const;

        private:
            Buffer(const Buffer& other);
            Buffer& operator=(const Buffer& other);

            unsigned alHandle;
    };


    struct ReverbEffectPreset;
    ReverbEffectPreset& GetHangarEffectPreset();

    class Effect {
        public:
            Effect();
            Effect(Effect&& other);
            ~Effect();

            //EAX reverb parameters
            void MakeReverb();
            void SetReverbDensity(float density);
            void SetReverbDiffusion(float diffusion);
            void SetReverbGain(float gain);
            void SetReverbGainHF(float gain);
            void SetReverbGainLF(float gain);
            void SetReverbDecayTime(float time);
            void SetReverbDecayHFRatio(float ratio);
            void SetReverbDecayLFRatio(float ratio);
            void SetReverbReflectionsGain(float gain);
            void SetReverbReflectionsDelay(float delay);
            void SetReverbLateReverbGain(float gain);
            void SetReverbLateReverbDelay(float delay);
            void SetReverbEchoTime(float time);
            void SetReverbEchoDepth(float depth);
            void SetReverbModulationTime(float time);
            void SetReverbModulationDepth(float depth);
            void SetReverbAirAbsorptionGainHF(float gain);
            void SetReverbHFReference(float reference);
            void SetReverbLFReference(float reference);
            void SetReverbDelayHFLimit(bool delay);

            void ApplyReverbPreset(ReverbEffectPreset& preset);

            operator unsigned() const;

        private:
            Effect(const Effect& other);
            Effect& operator=(const Effect& other);

            unsigned alHandle;
    };

    class EffectSlot {
        public:
            EffectSlot();
            EffectSlot(EffectSlot&& other);
            ~EffectSlot();

            void SetGain(float gain);
            void SetEffect(Effect& effect);

            operator unsigned() const;

        private:
            EffectSlot(const EffectSlot& other);
            EffectSlot& operator=(const EffectSlot& other);

            unsigned alHandle;
    };

    void SetListenerGain(float gain);
    void SetListenerPosition(const vec3_t position);
    void SetListenerVelocity(const vec3_t velocity);
    void SetListenerOrientation(const vec3_t orientation[3]);

    void SetSpeedOfSound(float speed);
    void SetDopplerExaggerationFactor(float factor);

    class Source {
        public:
            Source();
            Source(Source&& other);
            ~Source();

            void Play();
            void Pause();
            void Stop();
            bool IsStopped();

            void SetBuffer(Buffer& buffer);
            void QueueBuffer(Buffer buffer);
            int GetNumProcessedBuffers();
            int GetNumQueuedBuffers();
            Buffer PopBuffer();
            void RemoveAllQueuedBuffers();

            void SetGain(float gain);
            void SetPosition(const vec3_t position);
            void SetVelocity(const vec3_t velocity);
            void SetLooping(bool loop);
            void SetRolloff(float factor);
            void SetReferenceDistance(float distance);
            void SetRelative(bool relative);

            void SetSlotEffect(int slot, Effect& effect);
            void EnableSlot(int slot);
            void DisableSlot(int slot);

            operator unsigned() const;

        private:
            Source(const Source& other);
            Source& operator=(const Source& other);

            unsigned alHandle;
            EffectSlot slots[N_EFFECT_SLOTS];
    };

    class Device {
        public:
            static Device* FromName(Str::StringRef name);
            static Device* GetDefaultDevice();
            Device(Device&& other);
            ~Device();

            operator void*();

            static std::vector<std::string> ListByName();

        private:
            Device(void* alHandle);
            Device(const Device& other);
            Device& operator=(const Device& other);
            void* alHandle;
    };

    class Context {
        public:
            static Context* GetDefaultContext(Device& device);
            Context(Context&& other);
            ~Context();

            void MakeCurrent();

            operator void*();

        private:
            Context(void* alHandle);
            Context(const Context& other);
            Context& operator=(const Context& other);
            void* alHandle;
    };

}
}

#endif //AUDIO_AL_OBJECTS_H_
