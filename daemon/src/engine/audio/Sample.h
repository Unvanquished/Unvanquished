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

#ifndef AUDIO_SAMPLE_H_
#define AUDIO_SAMPLE_H_

namespace Audio {

    //TODO remove once we have VM handles
    template<typename T>
    class HandledResource {
        public:
            struct handleRecord_t {
                bool active;
                std::shared_ptr<T> value;
            };

            static bool IsValidHandle(int handle) {
                return handle >= 0 and (unsigned)handle < handles.size() and handles[handle].active;
            }
            static std::shared_ptr<T> FromHandle(int handle) {
                if (not IsValidHandle(handle)) {
                    return nullptr;
                }

                return handles[handle].value;
            }

            int GetHandle() {
                return handle;
            }

            void InitHandle(std::shared_ptr<T> self) {
                if (handle != -1) {
                    return;
                }

                if (not inactiveHandles.empty()) {
                    handle = inactiveHandles.back();
                    inactiveHandles.pop_back();
                    handles[handle] = {true, self};
                } else {
                    handle = handles.size();
                    handles.push_back({true, self});
                }
            }

        protected:
            HandledResource(): handle(-1) {
            }

            ~HandledResource() {
                if (IsValidHandle(handle)) {
                    handles[handle] = {false, nullptr};
                    inactiveHandles.push_back(handle);
                }
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

    class Sample: public HandledResource<Sample>, public Resource::Resource {
        public:
            explicit Sample(std::string name);
            virtual ~Sample() OVERRIDE FINAL;

            virtual bool Load() OVERRIDE FINAL;
            virtual void Cleanup() OVERRIDE FINAL;

            AL::Buffer& GetBuffer();

        private:
            AL::Buffer buffer;
    };

    void InitSamples();
    void ShutdownSamples();

	std::vector<std::string> ListSamples();

    void BeginSampleRegistration();
    std::shared_ptr<Sample> RegisterSample(Str::StringRef filename);
    void EndSampleRegistration();
}

#endif //AUDIO_SAMPLE_H_
