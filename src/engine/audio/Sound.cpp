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

    //TODO nice usecase for Cvar::Range
    //TODO lazily check for the values
    static Cvar::Cvar<float> effectsVolume("audio.volume.effects", "the volume of the effects", Cvar::ARCHIVE, 0.8f);
    static Cvar::Cvar<float> musicVolume("audio.volume.music", "the volume of the music", Cvar::ARCHIVE, 0.8f);

    // We have a big, fixed number of source to avoid rendering too many sounds and slowing down the rest of the engine.
    struct sourceRecord_t {
        AL::Source source;
        std::shared_ptr<Sound> usingSound;
        bool active;
        int priority;
    };

    static sourceRecord_t* sources = nullptr;
    static constexpr int nSources = 128; //TODO see what's the limit for OpenAL soft

    sourceRecord_t* GetSource(int priority);

    static bool initialized = false;

    void InitSounds() {
        if (initialized) {
            return;
        }

        sources = new sourceRecord_t[nSources];

        for (int i = 0; i < nSources; i++) {
            sources[i].active = false;
        }

        initialized = true;
    }

    void ShutdownSounds() {
        if (not initialized) {
            return;
        }

        delete[] sources;
        sources = nullptr;

        initialized = false;
    }

    void UpdateSounds() {
        for (int i = 0; i < nSources; i++) {
            if (sources[i].active) {
                auto sound = sources[i].usingSound;

                // Update and Emitter::UpdateSound can call Sound::Stop
                if (not sound->IsStopped()) {
                    sound->Update();
                }

                if (not sound->IsStopped()) {
                    sound->GetEmitter()->UpdateSound(*sound);
                }

                if (sound->IsStopped()) {
                    sources[i].active = false;
                    sources[i].usingSound = nullptr;
                }
            }
        }
    }

    void AddSound(std::shared_ptr<Emitter> emitter, std::shared_ptr<Sound> sound, int priority) {
        sourceRecord_t* source = GetSource(priority);

        if (source) {
            // Make the source forget if it was a "static" or a "streaming" source.
            source->source.ResetBuffer();
            sound->SetEmitter(emitter);
            sound->AcquireSource(source->source);
            source->usingSound = sound;
            source->priority = priority;
            source->active = true;

            sound->FinishSetup();
            sound->Play();
        }
    }

    // Finds a inactive or low-priority source to play a new sound.
    sourceRecord_t* GetSource(int priority) {
        //TODO make a better heuristic? (take into account the distance / the volume /... ?)
        int best = -1;
        int bestPriority = priority;

        // Gets the minimum sound by comparing activity first then priority
        for (int i = 0; i < nSources; i++) {
            sourceRecord_t& source = sources[i];

            if (not source.active) {
                return &source;
            }

            if (best < 0 && source.priority <= priority) {
                best = i;
                bestPriority = source.priority;
                continue;
            }

            if (source.priority < bestPriority) {
                best = i;
                bestPriority = source.priority;
                continue;
            }
        }

        if (best >= 0) {
            sourceRecord_t& source = sources[best];

            source.source.Stop();
            source.source.RemoveAllQueuedBuffers();

            source.usingSound = nullptr;
            return &source;
        } else {
            return nullptr;
        }
    }

    // Implementation of Sound

    Sound::Sound(): positionalGain(1.0f), soundGain(1.0f), currentGain(1.0f), playing(false), source(nullptr) {
    }

    Sound::~Sound() {
    }

    void Sound::Play() {
        source->Play();
        playing = true;
    }

    void Sound::Stop() {
        source->Stop();
        playing = false;
    }

    bool Sound::IsStopped() {
        return not playing;
    }

    void Sound::SetPositionalGain(float gain) {
        positionalGain = gain;
    }

    void Sound::SetSoundGain(float gain) {
        soundGain = gain;
    }

    float Sound::GetCurrentGain() {
        return currentGain;
    }

    void Sound::SetEmitter(std::shared_ptr<Emitter> emitter) {
        this->emitter = emitter;
    }

    std::shared_ptr<Emitter> Sound::GetEmitter() {
        return emitter;
    }

    void Sound::AcquireSource(AL::Source& source) {
        this->source = &source;

        source.SetLooping(false);

        SetupSource(source);
        emitter->SetupSound(*this);
    }

    AL::Source& Sound::GetSource() {
        return *source;
    }

    // Set the gain before the source is started to avoid having a few milliseconds of very lound sound
    void Sound::FinishSetup() {
        currentGain = positionalGain * soundGain * effectsVolume.Get();
        source->SetGain(currentGain);
    }

    void Sound::Update() {
        // Fade the Gain update to avoid "ticking" sounds when there is a gain discontinuity
        float targetGain = positionalGain * soundGain * effectsVolume.Get();

        //TODO make it framerate independant and fade out in about 1/8 seconds ?
        if (currentGain > targetGain) {
            currentGain = std::max(currentGain - 0.02f, targetGain);
            //currentGain = std::max(currentGain * 1.05f, targetGain);
        } else if (currentGain < targetGain) {
            currentGain = std::min(currentGain + 0.02f, targetGain);
            //currentGain = std::min(currentGain / 1.05f - 0.01f, targetGain);
        }

        source->SetGain(currentGain);

        InternalUpdate();
    }
    // Implementation of OneShotSound

    OneShotSound::OneShotSound(Sample* sample): sample(sample) {
    }

    OneShotSound::~OneShotSound() {
    }

    void OneShotSound::SetupSource(AL::Source& source) {
        source.SetBuffer(sample->GetBuffer());
        SetSoundGain(effectsVolume.Get());
    }

    void OneShotSound::InternalUpdate() {
        if (GetSource().IsStopped()) {
            Stop();
            return;
        }
        SetSoundGain(effectsVolume.Get());
    }

    // Implementation of LoopingSound

    LoopingSound::LoopingSound(Sample* sample): sample(sample), fadingOut(false) {
    }

    LoopingSound::~LoopingSound() {
    }

    void LoopingSound::FadeOutAndDie() {
        fadingOut = true;
        SetSoundGain(0.0f);
    }

    void LoopingSound::SetupSource(AL::Source& source) {
        source.SetLooping(true);
        source.SetBuffer(sample->GetBuffer());
        SetSoundGain(effectsVolume.Get());
    }

    void LoopingSound::InternalUpdate() {
        if (fadingOut and GetCurrentGain() == 0.0f) {
            Stop();
        }

        if (not fadingOut) {
            SetSoundGain(effectsVolume.Get());
        }
    }

    // Implementation of MusicSound

    MusicSound::MusicSound(Str::StringRef leadingStreamName, Str::StringRef loopStreamName)
        : leadingStream(S_CodecOpenStream(leadingStreamName.c_str())),
          loopStreamName(loopStreamName), loopStream(nullptr), playingLeadingSound(true){
    }

    MusicSound::~MusicSound() {
        if (leadingStream) {
            S_CodecCloseStream(leadingStream);
        }
        if (loopStream) {
            S_CodecCloseStream(loopStream);
        }
    }

    void MusicSound::SetupSource(AL::Source& source) {
        for (int i = 0; i < NUM_BUFFERS; i++) {
            AppendBuffer(source, std::move(AL::Buffer()));
        }
        SetSoundGain(musicVolume.Get());
    }

    void MusicSound::InternalUpdate() {
        AL::Source& source = GetSource();

        // Fill processed buffers and queue them back in the source
        while (source.GetNumProcessedBuffers() > 0) {
            AppendBuffer(source, std::move(source.PopBuffer()));
        }

        // If we don't have any more buffers queued it means we have to stop the music
        if (source.GetNumQueuedBuffers() == 0) {
            Stop();

        // If the source stopped because of a lack of buffer but we still have data to play we start the source again
        } else if (source.IsStopped()) {
            Play();
        }

        SetSoundGain(musicVolume.Get());
    }

    void MusicSound::AppendBuffer(AL::Source& source, AL::Buffer buffer) {
        snd_stream_t* streamToRead;
        if (playingLeadingSound and leadingStream) {
            streamToRead = leadingStream;
        } else if (loopStream) {
            streamToRead = loopStream;
        } else {
            return;
        }

        static char tempBuffer[CHUNK_SIZE];
        int lengthRead = S_CodecReadStream(streamToRead, CHUNK_SIZE, tempBuffer);

        // if a stream is finished, we start playing the loop again, if possible
        if (lengthRead == 0) {
            S_CodecCloseStream(streamToRead);
            playingLeadingSound = false;
            // either the stream was the leadin stream and we null it
            leadingStream = nullptr;
            // either it is the loop stream and we overwrite it
            loopStream = S_CodecOpenStream(loopStreamName.c_str());
            AppendBuffer(source, std::move(buffer));
            return;
        }

        snd_info_t streamInfo = streamToRead->info;
        streamInfo.size = lengthRead; // this isn't set by Codec
        buffer.Feed(streamInfo, tempBuffer);

        source.QueueBuffer(std::move(buffer));
    }

    // Implementation of StreamingSound

    StreamingSound::StreamingSound() {
    }

    StreamingSound::~StreamingSound() {
    }

    void StreamingSound::SetupSource(AL::Source&) {
    }

    void StreamingSound::InternalUpdate() {
        AL::Source& source = GetSource();

        while (source.GetNumProcessedBuffers() > 0) {
            source.PopBuffer();
        }

        if (source.GetNumQueuedBuffers() == 0) {
            Stop();
        }
    }

	//TODO somehow try to catch back when data is coming faster than we consume (e.g. capture data)
    void StreamingSound::AppendBuffer(AL::Buffer buffer) {
        if (IsStopped()) {
            return;
        }

        AL::Source& source = GetSource();
        source.QueueBuffer(std::move(buffer));

        if (source.IsStopped()) {
            source.Play();
        }
    }

    void StreamingSound::SetGain(float gain) {
        SetSoundGain(gain);
    }
}
