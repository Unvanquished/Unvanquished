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
#include "../../common/String.h"

#ifndef AUDIO_SAMPLE_H_
#define AUDIO_SAMPLE_H_

namespace Audio {

    template<typename T>
    class HandledResource {
        public:
            struct handleRecord_t {
                bool active;
                T* value;
            };

            static bool IsValidHandle(int handle) {
                return handle >= 0 and handle < handles.size() and handles[handle].active;
            }
            static T* FromHandle(int handle) {
                if (not IsValidHandle(handle)) {
                    return nullptr;
                }

                return handles[handle].value;
            }

            int GetHandle() {
                return handle;
            }

        protected:
            HandledResource(T* self) {
                if (not inactiveHandles.empty()) {
                    handle = inactiveHandles.back();
                    inactiveHandles.pop_back();
                    handles[handle] = {true, self};
                } else {
                    handle = handles.size();
                    handles.push_back({true, self});
                }
            }

            ~HandledResource() {
                handles[handle] = {false, nullptr};
                inactiveHandles.push_back(handle);
            }

        private:
            int handle;

            static std::vector<handleRecord_t> handles;
            static std::vector<int> inactiveHandles;
    };

    template<typename T>
    std::vector<typename HandledResource<T>::handleRecord_t> HandledResource<T>::handles;

    template<typename T>
    std::vector<int> HandledResource<T>::inactiveHandles;

    //TODO handle memory pressure: load and unload buffers automatically
    class Sample: public HandledResource<Sample> {
        public:
            Sample(std::string name);
            ~Sample();

            void Use();

            AL::Buffer& GetBuffer();

            void Retain();
            void Release();
            int GetRefCount();

            const std::string& GetName();

        private:
            AL::Buffer buffer;
            int refCount;
            std::string name;
    };

    void InitSamples();
    void ShutdownSamples();

    Sample* RegisterSample(Str::StringRef filename);
}

#endif //AUDIO_SAMPLE_H_
