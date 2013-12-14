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

namespace Audio {

    // Implementation of Sample

    Sample::Sample(std::string name): HandledResource<Sample>(this), refCount(0), name(std::move(name)) {
    }

    Sample::~Sample() {
    }

    void Sample::Use() {
    }

    AL::Buffer& Sample::GetBuffer() {
        return buffer;
    }

    void Sample::Retain() {
        refCount ++;
    }

    void Sample::Release() {
        refCount --;
    }

    int Sample::GetRefCount() {
        return refCount;
    }

    const std::string& Sample::GetName() {
        return name;
    }

    // Implementation of the sample storage

    static std::unordered_map<std::string, Sample*> samples;

    static Sample* errorSample = nullptr;
    bool initialized = false;

    void InitSamples() {
        if (initialized) {
            return;
        }
        //TODO make an error if it can't register
        //TODO use an aggressive sound instead to know we are missing something?
        errorSample = RegisterSample("sound/feedback/hit.wav");
        initialized = true;
    }

    void ShutdownSamples() {
        if (not initialized) {
            return;
        }

        //TODO

        initialized = false;
    }

    Sample* RegisterSample(Str::StringRef filename) {
        auto it = samples.find(filename);
        if (it != samples.end()) {
            return it->second;
        }

        snd_info_t info;
        void* data = S_CodecLoad(filename.c_str(), &info);

        if (data == nullptr) {
            Log::Warn("Couldn't load sound %s", filename);
            return errorSample;
        }

        Sample* sample = new Sample(std::move(filename));
        samples[filename] = sample;

        //TODO handle errors, especially out of memeory errors
        sample->GetBuffer().Feed(info, data);

        if (info.size == 0) {
            Log::Warn("info.size = 0 in RegisterSample, what?");
        }

        Hunk_FreeTempMemory(data);

        return sample;
    }
}
