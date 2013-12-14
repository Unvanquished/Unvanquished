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

#include "snd_local.h"
#include "snd_codec.h"

#include "../../common/String.h"

#ifndef AUDIO_API_H_
#define AUDIO_API_H_

namespace Audio {

    void Init();
    void Shutdown();
    void Update();

    sfxHandle_t RegisterSFX(Str::StringRef filename);

    void StartSound(int entityNum, const vec3_t origin, sfxHandle_t sfx);
    void StartLocalSound(int entityNum);

    void AddEntityLoopingSound(int entityNum, sfxHandle_t sfx);
    void ClearAllLoopingSounds();
    void ClearLoopingSoundsForEntity(int entityNum);

    void UpdateEntityPosition(int entityNum, const vec3_t position);
    void UpdateEntityVelocity(int entityNum, const vec3_t velocity);
    void UpdateEntityOcclusion(int entityNum, float ratio);
}

#endif //AUDIO_SOUND_H_
