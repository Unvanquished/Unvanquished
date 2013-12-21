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

#ifndef AUDIO_AUDIO_PRIVATE_H_
#define AUDIO_AUDIO_PRIVATE_H_

#include <unordered_map>
#include <memory>

#include "../../common/Log.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_codec.h"
#include "snd_public.h"

namespace Audio {

    /**
     * The audio system is split in several parts:
     * - Codecs, one for each supported format that allow to load a entire file or stream it from the disk.
     * - ALObjects that provide OO wrappers around OpenAL (OpenAL headers are only included in ALObjects.cpp)
     * - Audio the external interface, mostly using Sound and Emitter to create new sounds.
     * - Emitters that control the positional effects for the sound sources
     * - Sample that gives handles to loaded sound effects for use by the VM
     * - Sound that controls the raw sound shape emitted by a sound emitter (e.g. a looping sound, ...)
     */

    constexpr int POSITIONAL_EFFECT_SLOT = 0;

    // Somewhere on the Internet we can see "quake3 is like the old wolfenstein, 64 units = 8 feet"
    // it is consistent with our models and Carmack's being american.
    constexpr float QUNIT_IN_METER = 0.0384;

    // The speed of sound in qu/s
    constexpr float SPEED_OF_SOUND = 343.3 / QUNIT_IN_METER;
}

#include "ALObjects.h"
#include "Audio.h"
#include "Emitter.h"
#include "Sample.h"
#include "Sound.h"


#endif //AUDIO_AUDIO_PRIVATE_H_
