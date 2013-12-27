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

#ifndef AUDIO_EMITTER_H_
#define AUDIO_EMITTER_H_

namespace Audio {

    class Sound;
    class Emitter;

    void InitEmitters();
    void ShutdownEmitters();
    void UpdateEmitters();

    std::shared_ptr<Emitter> GetEmitterForEntity(int entityNum);
    std::shared_ptr<Emitter> GetEmitterForPosition(const vec3_t position);
    std::shared_ptr<Emitter> GetLocalEmitter();

    void UpdateListenerEntity(int entityNum, const vec3_t orientation[3]);
    void UpdateRegisteredEntityPosition(int entityNum, const vec3_t position);
    void UpdateRegisteredEntityVelocity(int entityNum, const vec3_t velocity);

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
            PositionEmitter(const vec3_t position);
            virtual ~PositionEmitter();

            void virtual Update() OVERRIDE;
            virtual void UpdateSound(Sound& sound) OVERRIDE;
            virtual void InternalSetupSound(Sound& sound) OVERRIDE;

            const vec3_t& GetPosition() const;

        private:
            vec3_t position;
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
