/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
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

#include <memory>
#include <unordered_map>

#include "../../common/Log.h"
#include "../../common/String.h"

#ifndef FRAMEWORK_RESOURCE_H_
#define FRAMEWORK_RESOURCE_H_

/*
 * Resource registration logic.
 *
 * Resources are any data that is loaded from the disk (sounds, images, ...).
 * The goal of the resource system impemented in this file is to allow:
 *  1 - resources to be loaded from the disk only if they aren't already loaded
 *  2 - to prevent duplicates of resources
 *  3 - resources to have dependencies on other resources (e.g. for shaders)
 *  4 - (TODO) allow asynchronous loading
 */

namespace Resource {

    template<typename T> class Manager;
    template<typename T> class Handle;

    /*
     * The interface that resources must implement to be used by the resource system.
     *
     * The resource loading is in three phases, first the Resource is instanciated
     * but it does mostly nothing, then TagDependencies is called that should load
     * from the disk only what is needed to know the dependencies of that resource
     * (for example shaders might depend on textures). Finally Load is called, that
     * does the actual loading of the resource from the disk. (there will be async
     * IO at some point)
     *
     * The data should be loaded from the end of Load and until the destructor is
     * called, the Resource::Manager is the one in charge of deleting the resource.
     */
    class Resource {
        public:
            // A resource is characterized by its name (usually a FS path)
            // almost nothing should happen in the resource constructor
            // especially no IO
            Resource(std::string name);
            virtual ~Resource();

            // Registers the dependencies of this resource, should return true on
            // success and false on error (in which case the resource will be deleted)
            // Defaults to []{return true;}
            virtual bool TagDependencies();

            // Loads the resource, doing potentially big IO, should return true on
            // success and false on error (in which case the resource will be deleted)
            // TODO provide a facility to know if resources we depend on have been loaded?
            virtual bool Load() = 0;

            virtual void Cleanup() = 0;

            // Checks if the resource is still valid and up to date,
            // for example after the FS loads a mod, some resources can have
            // been modified by the mod and what's in memory isn't valid anymore.
            // Defaults to []{return true;}
            virtual bool IsStillValid();

            // Returns the name of the resource.
            Str::StringRef GetName();

        private:
            bool TryLoad();

            std::string name;

            bool loaded;
            bool failed;

            //TODO remove .keep once we have VM handles
            // It is needed for now because the VM cannot ask for a shared_ptr so it
            // doesn't add a refcount. .keep is a hack to avoid deleting resources
            // added during the registration.
            bool keep;

            template<typename T> friend class Manager;
            template<typename T> friend class Handle;
    };

    template<typename T>
    class Handle {
        public:
            Handle(std::shared_ptr<T> value, const Manager<T>* manager): value(value), manager(manager) {
            }

            std::shared_ptr<T> Get() {
                if (value->failed) {
                    value = manager->GetDefaultValue();
                }
                return value;
            }
        private:
            std::shared_ptr<T> value;
            const Manager<T>* manager;
    };

    /*
     * Manages resources so as to avoid redundant loads as well as defer the loading
     * to prepare for async loading. The manager can be in two states, either in the
     * registration during which the loading of resources will be defered, either not
     * in the registration in which case resources will be loaded immediately.
     *
     * The manager gives shared_ptr's of resources, a resource will be candidate for
     * deletion when the manager has the last pointer to the resource (so that we are
     * sure that noone is currently using the resource).
     */
    template<typename T>
    class Manager {
        private:
            typedef typename std::unordered_map<Str::StringRef, std::shared_ptr<T>>::iterator iterator;

        public:
            Manager(Str::StringRef name = "");
            ~Manager();

            // Starts the registration.
            void BeginRegistration();

            // Ends the registration.
            void EndRegistration();

            // Register the resource, returns a shared_ptr to that resource or nullptr
            // on a fail.
            Handle<T> Register(Str::StringRef name);

            // Search and delete unused resources.
            void Prune();

            // Allow the for (auto& resource: myResourceManager) construct to iterate
            // over all the resources of that manager. There must not be any other
            // operation on the manager during the iteration.
            iterator begin();
            iterator end();

            std::shared_ptr<T> GetDefaultValue() const {
                return defaultValue;
            }

        private:
            bool inRegistration;
            std::shared_ptr<T> defaultValue;
            // We store a StringRef to the resource's name as we know that the lifetime
            // of the resource will be longer than the one of the hashmap entry.
            std::unordered_map<Str::StringRef, std::shared_ptr<T>> resources;
    };

    // Implementation of the templates

    template<typename T>
    Manager<T>::Manager(Str::StringRef defaultName): inRegistration(false) {
        if (defaultName == "") {
            defaultValue = nullptr;
        } else {
            defaultValue = Register(defaultName).Get();
            if (not defaultValue) {
                Log::Error("Couldn't load the default resource for %s\n", typeid(T).name());
            }
        }
    }

    template<typename T>
    Manager<T>::~Manager() {
        //TODO assert that we have the ownership of all the resources?
    }

    template<typename T>
    void Manager<T>::BeginRegistration() {
        for (auto& entry : resources) {
            entry.second->keep = false;
        }

        inRegistration = true;
    }

    template<typename T>
    void Manager<T>::EndRegistration() {
        // Delete unused resources
        Prune();

        // And then load the new ones, so as to reduce peak memory usage.
        for (auto it = resources.begin(); it != resources.end(); it ++) {
            if (not it->second->loaded) {
                if (not it->second->TryLoad()) {
                    it->second->Cleanup();
                    it = resources.erase(it);
                }
            }
        }

        inRegistration = false;
    }

    template<typename T>
    Handle<T> Manager<T>::Register(Str::StringRef name) {
        auto it = resources.find(name);

        if (it != resources.end()) {
            it->second->keep = true;
            return Handle<T>(it->second, this);
        }

        std::shared_ptr<T> newResource = std::make_shared<T>(std::move(name));
        if (not newResource->TagDependencies()) {
            return Handle<T>(defaultValue, this);
        }

        if (inRegistration) {
            resources[newResource->GetName()] = newResource;
        } else {
            if (newResource->TryLoad()) {
                resources[newResource->GetName()] = newResource;
            } else {
                return Handle<T>(defaultValue, this);
            }
        }

        return Handle<T>(newResource, this);
    }

    template<typename T>
    void Manager<T>::Prune() {
        for (auto it = resources.begin(); it != resources.end(); it ++) {
            if (not it->second->keep and it->second.unique()) {
                it->second->Cleanup();
                it = resources.erase(it);
            }
        }
    }

    template<typename T>
    typename Manager<T>::iterator Manager<T>::begin() {
        return resources.begin();
    }

    template<typename T>
    typename Manager<T>::iterator Manager<T>::end() {
        return resources.end();
    }
}

#endif //FRAMEWORK_RESOURCE_H_
