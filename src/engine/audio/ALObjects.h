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

#include <al.h>
#include <alc.h>
#include "snd_local.h"
#include "snd_codec.h"

#ifndef AUDIO_AL_OBJECTS_H_
#define AUDIO_AL_OBJECTS_H_

namespace Audio {
namespace AL {

    //TODO enum classes for the different ALuint types?

    void ClearError();

    //TODO remove the need for this somehow
    ALuint Format(int width, int channels);

    class Buffer {
        public:
            Buffer();
            Buffer(Buffer&& other);
            ~Buffer();

            ALuint Feed(snd_info_t info, void* data);

            operator ALuint() const;

        private:
            Buffer(const Buffer& other);
            Buffer& operator=(const Buffer& other);

            ALuint alHandle;
    };

    class Source {
        public:
            Source();
            Source(Source&& other);
            ~Source();

            void Play();
            void Pause();
            void Stop();

            bool IsStopped();

            void SetGain(float gain);
            void SetPosition(const vec3_t position);
            void SetVelocity(const vec3_t velocity);
            void SetBuffer(Buffer& buffer);
            void SetLooping(bool loop);
            void SetRolloff(float factor);
            void SetReferenceDistance(float distance);
            void SetRelative(bool relative);

            operator ALuint() const;

        private:
            Source(const Source& other);
            Source& operator=(const Source& other);

            ALuint alHandle;
    };

}
}

#endif //AUDIO_AL_OBJECTS_H_
