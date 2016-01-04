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

#include "AudioPrivate.h"
#include "SoundCodec.h"

namespace Audio {

    Resource::Manager<Sample>* sampleManager;

    // Implementation of Sample

    Sample::Sample(std::string filename): Resource(filename) {
    }

    Sample::~Sample() {
        audioLogs.Debug("Deleting Sample '%s'", GetName());
    }

    bool Sample::Load() {
        audioLogs.Debug("Loading Sample '%s'", GetName());
	    auto audioData = LoadSoundCodec(GetName());

	    if (audioData.size == 0) {
		    audioLogs.Warn("Couldn't load sound %s, it's empty!", GetName());
            return false;
        }

        //TODO handle errors, especially out of memory errors
        buffer.Feed(audioData);

	    return true;
    }

    void Sample::Cleanup() {
        // Destroy the OpenAL buffer by moving it in the scope
        AL::Buffer toDelete = std::move(buffer);
    }

    AL::Buffer& Sample::GetBuffer() {
        return buffer;
    }

    // Implementation of the sample storage

    static const char errorSampleName[] = "sound/feedback/hit.wav";
    static std::shared_ptr<Sample> errorSample = nullptr;
    bool initialized = false;

    void InitSamples() {
        if (initialized) {
            return;
        }

        sampleManager = new Resource::Manager<Sample>(errorSampleName);

        // Work around for the lack of VM Handles, initiliaze the HandledResource
        auto errorSample = sampleManager->GetResource(errorSampleName).Get();
        errorSample->InitHandle(errorSample);

        initialized = true;
    }

    void ShutdownSamples() {
        if (not initialized) {
            return;
        }

        errorSample = nullptr;

        delete sampleManager;
        sampleManager = nullptr;

        initialized = false;
    }

	std::vector<std::string> ListSamples() {
		if (not initialized) {
			return {};
		}

		std::vector<std::string> res;

		for (auto& it : *sampleManager) {
			res.push_back(it.first);
		}

		return res;
	}

    void BeginSampleRegistration() {
        sampleManager->BeginRegistration();
    }

    std::shared_ptr<Sample> RegisterSample(Str::StringRef filename) {
        Resource::Handle<Sample> sample = sampleManager->Register(filename);
        // Work around for the lack of VM Handles, initiliaze the HandledResource
        sample.Get()->InitHandle(sample.Get());
        return sample.Get();
    }

    void EndSampleRegistration() {
        sampleManager->EndRegistration();
    }
}
