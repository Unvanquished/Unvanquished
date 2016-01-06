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

#ifndef AUDIO_EMITTER_H_
#define AUDIO_EMITTER_H_

namespace Audio {

    class Sound;
    class Emitter;

    void InitEmitters();
    void ShutdownEmitters();
    void UpdateEmitters();

    std::shared_ptr<Emitter> GetEmitterForEntity(int entityNum);
    std::shared_ptr<Emitter> GetEmitterForPosition(Vec3 position);
    std::shared_ptr<Emitter> GetLocalEmitter();

    void UpdateListenerEntity(int entityNum, const Vec3 orientation[3]);
    void UpdateRegisteredEntityPosition(int entityNum, Vec3 position);
    void UpdateRegisteredEntityVelocity(int entityNum, Vec3 velocity);

    void UpdateReverbSlot(int slotNum, std::string name, float ratio);

    class Sound;

    namespace AL {
        class Source;
    }

    class Emitter {
        public:
            Emitter();
            virtual ~Emitter();

            void SetupSound(Sound& sound);

            // Called each frame before any UpdateSound is called, used to factor computations
            void virtual Update() = 0;
            // Update the Sound's source's spatialization
            virtual void UpdateSound(Sound& sound) = 0;
            // Setup a source for the spatialization of this Emitter
            virtual void InternalSetupSound(Sound& sound) = 0;
    };

    // An Emitter that will follow an entity
    class EntityEmitter : public Emitter {
        public:
            EntityEmitter(int entityNum);
            virtual ~EntityEmitter();

            void virtual Update() OVERRIDE;
            virtual void UpdateSound(Sound& sound) OVERRIDE;
            virtual void InternalSetupSound(Sound& sound) OVERRIDE;

        private:
            int entityNum;
    };

    // An Emitter at a fixed position in space
    class PositionEmitter : public Emitter {
        public:
            PositionEmitter(Vec3 position);
            virtual ~PositionEmitter();

            void virtual Update() OVERRIDE;
            virtual void UpdateSound(Sound& sound) OVERRIDE;
            virtual void InternalSetupSound(Sound& sound) OVERRIDE;

            Vec3 GetPosition() const;

        private:
            Vec3 position;
    };

    // An Emitter for things that aren't spatialized (like menus, annoucements, ...)
    class LocalEmitter: public Emitter {
        public:
            LocalEmitter();
            virtual ~LocalEmitter();

            void virtual Update() OVERRIDE;
            virtual void UpdateSound(Sound& sound) OVERRIDE;
            virtual void InternalSetupSound(Sound& sound) OVERRIDE;
    };

}

#endif //AUDIO_SAMPLE_H_
