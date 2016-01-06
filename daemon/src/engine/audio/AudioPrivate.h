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

#include "framework/Resource.h"
#include "qcommon/qcommon.h"

#ifndef AUDIO_AUDIO_PRIVATE_H_
#define AUDIO_AUDIO_PRIVATE_H_

namespace Audio {

    /**
     * The audio system is split in several parts:
     * - Audio codecs, one for each supported format that allow to load an entire file.
     * - ALObjects that provide OO wrappers around OpenAL (OpenAL headers are only included in ALObjects.cpp)
     * - Audio the external interface, mostly using Sound and Emitter to create new sounds.
     * - Emitters that control the positional effects for the sound sources
     * - Sample that gives handles to loaded sound effects for use by the VM
     * - Sound that controls the raw sound shape emitted by a sound emitter (e.g. a looping sound, ...)
     *
     * In term of ownership, Samples are owned by the hashmap filename <-> Samples, OpenAL sources
     * are allocated in an array in Sound and each source can have at most one sound. Each sound has one
     * Emitter (they are ref counted).
     */

    // Somewhere on the Internet we can see "quake3 is like the old wolfenstein, 64 units = 8 feet"
    // it is consistent with our models and Carmack's being american.
    CONSTEXPR float QUNIT_IN_METER = 0.0384;

    // The speed of sound in qu/s
    CONSTEXPR float SPEED_OF_SOUND = 343.3 / QUNIT_IN_METER;

    // Same number of raw streams as in the previous sound system
    CONSTEXPR int N_STREAMS = MAX_CLIENTS * 2 + 1;

    // There is only a small number of reverb slots because by default we can create only 4 AuxEffects
    CONSTEXPR int N_REVERB_SLOTS = 3;

    // Tweaks the value given by the audio slider
    float SliderToAmplitude(float slider);

    extern Log::Logger audioLogs;
}

#include "ALObjects.h"
#include "Audio.h"
#include "Emitter.h"
#include "Sample.h"
#include "Sound.h"


#endif //AUDIO_AUDIO_PRIVATE_H_
