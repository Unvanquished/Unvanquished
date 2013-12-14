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

#ifndef AUDIO_SOUND_H_
#define AUDIO_SOUND_H_

namespace Audio {

    class Emitter;
    class Sound;

    void InitSounds();
    void ShutdownSounds();
    void UpdateSounds();

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
            void Stop();
            bool IsStopped();

            void SetEmitter(std::shared_ptr<Emitter> emitter);
            std::shared_ptr<Emitter> GetEmitter();

            void AcquireSource(AL::Source& source);
            AL::Source& GetSource();

            virtual void SetupSource(AL::Source& source) = 0;
            virtual void Update() = 0;

        private:
            bool playing;
            std::shared_ptr<Emitter> emitter;
            AL::Source* source;
    };

    class OneShotSound : public Sound {
        public:
            OneShotSound(Sample* sample);
            virtual ~OneShotSound();

            virtual void SetupSource(AL::Source& source) OVERRIDE;
            virtual void Update() OVERRIDE;

        private:
            Sample* sample;
    };


    class LoopingSound : public Sound {
        public:
            LoopingSound(Sample* sample);
            virtual ~LoopingSound();

            virtual void SetupSource(AL::Source& source) OVERRIDE;
            virtual void Update() OVERRIDE;

        private:
            Sample* sample;
    };

    // StreamSound
    // MusicSound?

}

#endif //AUDIO_SOUND_H_
