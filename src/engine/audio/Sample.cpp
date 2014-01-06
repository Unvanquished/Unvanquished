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

    static const char errorSampleName[] = "sound/feedback/hit.wav";
    static Sample* errorSample = nullptr;
    bool initialized = false;

    void InitSamples() {
        if (initialized) {
            return;
        }
        //TODO use an aggressive sound instead to know we are missing something?
        errorSample = RegisterSample(errorSampleName);

        if (!errorSample) {
            Com_Error(ERR_FATAL, "Couldn't load the error sound sample '%s'", errorSampleName);
        }

        initialized = true;
    }

    void ShutdownSamples() {
        if (not initialized) {
            return;
        }

        for (auto it: samples) {
            delete it.second;
        }
        samples.clear();

        errorSample = nullptr;

        initialized = false;
    }

	std::vector<std::string> ListSamples() {
		if (not initialized) {
			return {};
		}

		std::vector<std::string> res;

		for (auto& it : samples) {
			res.push_back(it.first);
		}

		return res;
	}

    Sample* RegisterSample(Str::StringRef filename) {
        auto it = samples.find(filename);
        if (it != samples.end()) {
            return it->second;
        }

        snd_info_t info;
        void* data = S_CodecLoad(filename.c_str(), &info);

        if (data == nullptr) {
            audioLogs.Warn("Couldn't load sound %s", filename);
            return errorSample;
        }

        Sample* sample = new Sample(std::move(filename));
        samples[filename] = sample;

        //TODO handle errors, especially out of memeory errors
        sample->GetBuffer().Feed(info, data);

        if (info.size == 0) {
            audioLogs.Warn("info.size = 0 in RegisterSample, what?");
        }

        Hunk_FreeTempMemory(data);

        return sample;
    }
}
