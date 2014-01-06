/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef AUDIO_SOUND_H_
#define AUDIO_SOUND_H_

namespace Audio {

    class Emitter;
    class Sound;

    void InitSounds();
    void ShutdownSounds();
    void UpdateSounds();

    // The only way to add a sound, attaches the sound to the emitter, a higher priority means
    // the sound will be less likely to be recycled to spawn a new sound.
    void AddSound(std::shared_ptr<Emitter> emitter, std::shared_ptr<Sound> sound, int priority);

    class Sample;

    namespace AL {
        class Source;
    }

    //TODO sound.mute
    class Sound {
        public:
            Sound();
            virtual ~Sound();

            void Play();
            // Stop the source and marks the sound for deletion.
            void Stop();
            bool IsStopped();

            // The is attenuated because of its inherent porperties and because of its position.
            // Each attenuation can be set separately.
            void SetPositionalGain(float gain);
            void SetSoundGain(float gain);
            float GetCurrentGain();

            void SetEmitter(std::shared_ptr<Emitter> emitter);
            std::shared_ptr<Emitter> GetEmitter();

            void AcquireSource(AL::Source& source);
            AL::Source& GetSource();

            // Used to setup a source for a specific kind of sound and to start the sound.
            virtual void SetupSource(AL::Source& source) = 0;
            void FinishSetup();

            void Update();
            // Called each frame, after emitters have been updated.
            virtual void InternalUpdate() = 0;

        private:
            float positionalGain;
            float soundGain;
            float currentGain;

            bool playing;
            std::shared_ptr<Emitter> emitter;
            AL::Source* source;
    };

    // A sound that is played once.
    class OneShotSound : public Sound {
        public:
            OneShotSound(Sample* sample);
            virtual ~OneShotSound();

            virtual void SetupSource(AL::Source& source) OVERRIDE;
            virtual void InternalUpdate() OVERRIDE;

        private:
            Sample* sample;
    };

    // A looping sound
    class LoopingSound : public Sound {
        public:
            LoopingSound(Sample* sample);
            virtual ~LoopingSound();

            void FadeOutAndDie();

            virtual void SetupSource(AL::Source& source) OVERRIDE;
            virtual void InternalUpdate() OVERRIDE;

        private:
            Sample* sample;
            bool fadingOut;
    };

    // A sound that is too big to be loaded al at once in the memory: it is instead streamed from the disc.
    class MusicSound : public Sound {
        public:
            // Both names are optional
            MusicSound(Str::StringRef leadingStreamName, Str::StringRef loopStreamName);
            virtual ~MusicSound();

            virtual void SetupSource(AL::Source& source) OVERRIDE;
            virtual void InternalUpdate() OVERRIDE;

        private:
            void AppendBuffer(AL::Source& source, AL::Buffer buffer);

            snd_stream_t* leadingStream;

            std::string loopStreamName;
            snd_stream_t* loopStream;

            bool playingLeadingSound;

            // We only need three buffers in order to we have at all time a full buffer queued
            // the problematic case is when we have a buffer buffering the last few samples of
            // the leading sound or of the end of the loop.
            static const int NUM_BUFFERS = 4;

            // Buffer about a tenth of a second in each chunk (with 16 bit stereo at 44kHz)
            static const int CHUNK_SIZE = 16384;
    };

    // Any sound that receives its data over time (such as VoIP)
    class StreamingSound : public Sound {
        public:
            StreamingSound();
            virtual ~StreamingSound();

            virtual void SetupSource(AL::Source& source) OVERRIDE;
            virtual void InternalUpdate() OVERRIDE;

            void AppendBuffer(AL::Buffer buffer);
            void SetGain(float gain);
    };

}

#endif //AUDIO_SOUND_H_
