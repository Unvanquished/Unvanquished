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

#include "ALObjects.h"
#include "../../common/Log.h"

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

    ALuint Buffer::Feed(snd_info_t info, void* data) {
        ALuint format = Format(info.width, info.channels);

        ClearError();
		alBufferData(alHandle, format, data, info.size, info.rate);

        return alGetError();
    }

    Buffer::operator ALuint() const {
        return alHandle;
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

    Source::operator ALuint() const {
        return alHandle;
    }

}
}
