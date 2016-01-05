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

#ifndef AUDIO_AL_OBJECTS_H_
#define AUDIO_AL_OBJECTS_H_

#include "AudioData.h"

namespace Audio {
namespace AL {

    /**
     * Provides wrappers around OpenAL objects. Good resources to understand OpenAL are:
     * - The original OpenAL programmer's guide
     * - The OpenAL Effects Extension Guide
     * - The OpenAL Soft source code when in doubt
     * - This source code?
     *
     * The classes defined here are non-copyable but movable, consider them as
     * unique_ptr's to the OpenAL object.
     *
     * To Debug OpenAL errors, you can set ALSOFT_TRAP_ERROR=1 in the environment and
     * OpenAL will break in the debugger on any error. Alternatively you can set
     * ALSOFT_LOGFILE and ALSOFT_LOGLEVEL(0 to 3) to get more info.
     */

    //TODO enum classes for the different ALuint types?

    // Contains raw data to be played by an OpenAL source
    class Buffer {
        public:
            Buffer();
            Buffer(Buffer&& other);
            ~Buffer();

            // TODO
            // Fills the buffer with data (width/rate/size should be given by info)
	        unsigned Feed(const AudioData& audioData);

	        // Both these methods are use by Source to Queue/Unqueue buffers
            Buffer(unsigned handle);
            unsigned Acquire();

            operator unsigned() const;

        private:
            Buffer(const Buffer& other);
            Buffer& operator=(const Buffer& other);

            unsigned alHandle;
    };

    struct ReverbEffectPreset;
    void InitEffectPresets();
    ReverbEffectPreset* GetPresetByName(Str::StringRef name);
    std::vector<std::string> ListPresetNames();

    /**
     * OpenAL's EFX extension adds support for effects (notably reverb). Each OpenAL
     * source can send to the main "mixer" as well as to a limited number of global effect mixers
     * (4 in total in most OpenAL implementations) that are called Auxiliary Effect Slots
     * (EffectSlots here). The output from these mixers is then mixed in the main "mixer".
     * The Effects OpenAL object is merely a place to build parameters for an EffectSlot.
     * In addition the outputs of a source to the main "mixer" and the EffectSlots can be applied
     * independantly to a simple Filter (LF/BandF/HF).
     */

    // The parameters of an effect.
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

            //Avoid much typing and parameter-guessing by loading a preset.
            void ApplyReverbPreset(ReverbEffectPreset& preset);

            operator unsigned() const;

        private:
            Effect(const Effect& other);
            Effect& operator=(const Effect& other);

            unsigned alHandle;
    };

    // A mixer that receives signal from multiple sources and applis an effect.
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

    // The Listener Gain is the global OpenAL Gain
    void SetListenerGain(float gain);
    void SetListenerPosition(Vec3 position);
    void SetListenerVelocity(Vec3 velocity);
    void SetListenerOrientation(const Vec3 orientation[3]);

    void SetInverseDistanceModel();
    void SetSpeedOfSound(float speed);
    void SetDopplerExaggerationFactor(float factor);

    // Plays a sound with a lot of parameters (including spatialization)
    class Source {
        public:
            Source();
            Source(Source&& other);
            ~Source();

            void Play();
            void Pause();
            void Stop();
            bool IsStopped();

            // Use this buffer and only this buffer to get the sound data. Sets the source state as STATIC.
            // Does not consume the buffer.
            void SetBuffer(Buffer& buffer);

            // Appends the buffer to the list of sounds to be queued. Consumes the buffer.
            void QueueBuffer(Buffer buffer);
            // The number of buffers that can be asked back using PopBuffer
            int GetNumProcessedBuffers();
            int GetNumQueuedBuffers();
            // Returns one of the buffers that was queued in the source and that is not used anymore.
            Buffer PopBuffer();
            // Destroys all the buffers queued in the source, effectively stopping the source.
            void RemoveAllQueuedBuffers();

            // Reset the STATIC or STREAMING state (causes OpenAL errors for example when queueing on a STREAMING source)
            void ResetBuffer();

            void SetGain(float gain);
            void SetPosition(Vec3 position);
            void SetVelocity(Vec3 velocity);
            void SetLooping(bool loop);
            void SetRolloff(float factor);
            void SetReferenceDistance(float distance);
            void SetRelative(bool relative);

            // Binds <effect> to the exit wire number <slot> of the source. This is called an Auxiliary Send in OpenAL
            void EnableEffect(int slot, EffectSlot& effect);
            void DisableEffect(int slot);

            operator unsigned() const;

        private:
            Source(const Source& other);
            Source& operator=(const Source& other);

            int GetType();

            unsigned alHandle;
    };

    // The hardware that will play our sounds.
    class Device {
        public:
            static Device* FromName(Str::StringRef name);
            static Device* GetDefaultDevice();
            Device(Device&& other);
            ~Device();

            operator void*();

            static std::string DefaultDeviceName();
            static std::vector<std::string> ListByName();

        private:
            Device(void* alHandle);
            Device(const Device& other);
            Device& operator=(const Device& other);
            void* alHandle;
    };

    // Tne OpenAL context.
    class Context {
        public:
            static Context* GetDefaultContext(Device& device);
            Context(Context&& other);
            ~Context();

            // Sets the current OpenAL thread.
            void MakeCurrent();

            operator void*();

        private:
            Context(void* alHandle);
            Context(const Context& other);
            Context& operator=(const Context& other);
            void* alHandle;
    };

    class CaptureDevice {
        public:
            static CaptureDevice* FromName(Str::StringRef name, int rate);
            static CaptureDevice* GetDefaultDevice(int rate);
            CaptureDevice(CaptureDevice&& other);
            ~CaptureDevice();

            void Start();
            void Stop();
            int GetNumCapturedSamples();
            void Capture(int numSamples, void* buffer);

            operator void*();

            static std::string DefaultDeviceName();
            static std::vector<std::string> ListByName();

        private:
            CaptureDevice(void* alHandle);
            CaptureDevice(const Device& other);
            CaptureDevice& operator=(const CaptureDevice& other);
            void* alHandle;
    };

    std::string GetSystemInfo(Device* device, CaptureDevice* capture);

}
}

#endif //AUDIO_AL_OBJECTS_H_
